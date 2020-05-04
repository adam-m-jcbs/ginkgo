/*******************************<GINKGO LICENSE>******************************
Copyright (c) 2017-2020, the Ginkgo authors
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
******************************<GINKGO LICENSE>*******************************/


#include <ginkgo/ginkgo.hpp>


#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>


int main(int argc, char *argv[])
{
    // Some shortcuts
    using ValueType = double;
    using IndexType = int;

    using vec = gko::matrix::Dense<ValueType>;
    using mtx = gko::matrix::Csr<ValueType, IndexType>;
    using gmres = gko::solver::Gmres<ValueType>;
    using ir = gko::solver::Ir<ValueType>;
    using bj = gko::preconditioner::Jacobi<ValueType, IndexType>;

    // Print version information
    std::cout << gko::version_info::get() << std::endl;

    // Figure out where to run the code
    std::shared_ptr<gko::Executor> exec;
    if (argc == 1 || std::string(argv[1]) == "reference") {
        exec = gko::ReferenceExecutor::create();
    } else if (argc == 2 && std::string(argv[1]) == "omp") {
        exec = gko::OmpExecutor::create();
    } else if (argc == 2 && std::string(argv[1]) == "cuda" &&
               gko::CudaExecutor::get_num_devices() > 0) {
        exec = gko::CudaExecutor::create(0, gko::OmpExecutor::create());
    } else if (argc == 2 && std::string(argv[1]) == "hip" &&
               gko::HipExecutor::get_num_devices() > 0) {
        exec = gko::HipExecutor::create(0, gko::OmpExecutor::create());
    } else {
        std::cerr << "Usage: " << argv[0] << " [executor]" << std::endl;
        std::exit(-1);
    }

    // Read data
    auto A = gko::share(gko::read<mtx>(std::ifstream("data/A.mtx"), exec));
    auto b = gko::read<vec>(std::ifstream("data/b.mtx"), exec);
    auto x = gko::read<vec>(std::ifstream("data/x0.mtx"), exec);

    // Generate incomplete factors using ParILU
    auto par_ilu_fact =
        gko::factorization::ParIlu<ValueType, IndexType>::build().on(exec);
    // Generate concrete factorization for input matrix
    auto par_ilu = par_ilu_fact->generate(A);

    // Generate an iterative refinement factory to be used as triangular solver
    // in the preconditioner application. The generated method is equivalent
    // to doing five block-Jacobi sweeps with maximum block size 16.
    auto trisolve_factory =
        ir::build()
            .with_solver(bj::build()
                             .with_max_block_size(16u)
                             .with_storage_optimization(
                                 gko::precision_reduction::autodetect())
                             .on(exec))
            .with_criteria(
                gko::stop::Iteration::build().with_max_iters(5u).on(exec))
            .on(exec);

    // Generate an ILU preconditioner factory by setting lower and upper
    // triangular solver - in this case the previously defined iterative
    // refinement method.
    auto ilu_pre_factory =
        gko::preconditioner::Ilu<ir, ir, false>::build()
            .with_l_solver_factory(gko::clone(trisolve_factory))
            .with_u_solver_factory(gko::clone(trisolve_factory))
            .on(exec);

    // Use incomplete factors to generate ILU preconditioner
    auto ilu_preconditioner = ilu_pre_factory->generate(gko::share(par_ilu));

    // Use preconditioner inside GMRES solver factory
    // Generating a solver factory tied to a specific preconditioner makes sense
    // if there are several very similar systems to solve, and the same
    // solver+preconditioner combination is expected to be effective.
    const gko::remove_complex<ValueType> reduction_factor = 1e-12;
    auto ilu_gmres_factory =
        gmres::build()
            .with_criteria(
                gko::stop::Iteration::build().with_max_iters(1000u).on(exec),
                gko::stop::ResidualNormReduction<ValueType>::build()
                    .with_reduction_factor(reduction_factor)
                    .on(exec))
            .with_generated_preconditioner(gko::share(ilu_preconditioner))
            .on(exec);

    // Generate preconditioned solver for a specific target system
    auto ilu_gmres = ilu_gmres_factory->generate(A);

    // Solve system
    ilu_gmres->apply(gko::lend(b), gko::lend(x));

    // Print solution
    std::cout << "Solution (x): \n";
    write(std::cout, gko::lend(x));

    // Calculate residual
    auto one = gko::initialize<vec>({1.0}, exec);
    auto neg_one = gko::initialize<vec>({-1.0}, exec);
    auto res = gko::initialize<vec>({0.0}, exec);
    A->apply(gko::lend(one), gko::lend(x), gko::lend(neg_one), gko::lend(b));
    b->compute_norm2(gko::lend(res));

    std::cout << "Residual norm sqrt(r^T r): \n";
    write(std::cout, gko::lend(res));
}
