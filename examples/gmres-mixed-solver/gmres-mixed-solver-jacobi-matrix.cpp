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

// @sect3{Include files}

// This is the main ginkgo header file.
#include <ginkgo/ginkgo.hpp>
//#include "cuda/components/zero_array.hpp"

// Add the fstream header to read from data from files.
#include <fstream>
// Add the C++ iostream header to output information to the console.
#include <iostream>
// Add the string manipulation header to handle strings.
#include <string>


int main(int argc, char *argv[])
{
    // Use some shortcuts. In Ginkgo, vectors are seen as a gko::matrix::Dense
    // with one column/one row. The advantage of this concept is that using
    // multiple vectors is a now a natural extension of adding columns/rows are
    // necessary.
    using vec = gko::matrix::Dense<>;
    // The gko::matrix::Csr class is used here, but any other matrix class such
    // as gko::matrix::Coo, gko::matrix::Hybrid, gko::matrix::Ell or
    // gko::matrix::Sellp could also be used.
    using mtx = gko::matrix::Csr<>;
    // The gko::solver::Cg is used here, but any other solver class can also be
    // used.
    //    using gmres_mixed = gko::solver::GmresMixed<double, float>;
    using gmres_mixed = gko::solver::GmresMixed<>;
    using bj = gko::preconditioner::Jacobi<>;

    // Print the ginkgo version information.
    std::cout << gko::version_info::get() << std::endl;

    // @sect3{Where do you want to run your solver ?}
    // The gko::Executor class is one of the cornerstones of Ginkgo. Currently,
    // we have support for
    // an gko::OmpExecutor, which uses OpenMP multi-threading in most of its
    // kernels, a gko::ReferenceExecutor, a single threaded specialization of
    // the OpenMP executor and a gko::CudaExecutor which runs the code on a
    // NVIDIA GPU if available.
    // @note With the help of C++, you see that you only ever need to change the
    // executor and all the other functions/ routines within Ginkgo should
    // automatically work and run on the executor with any other changes.
    /*
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
    */
    int index_matrix = -1;
    std::shared_ptr<gko::Executor> exec;
    if (argc == 1 || std::string(argv[1]) == "reference") {
        exec = gko::ReferenceExecutor::create();
        if (argc == 3) index_matrix = 2;
    } else if (argc <= 3 && std::string(argv[1]) == "omp") {
        exec = gko::OmpExecutor::create();
        if (argc == 3) index_matrix = 2;
    } else if (argc <= 3 && std::string(argv[1]) == "cuda" &&
               gko::CudaExecutor::get_num_devices() > 0) {
        exec = gko::CudaExecutor::create(0, gko::OmpExecutor::create());
        if (argc == 3) index_matrix = 2;
    } else if (argc <= 3 && std::string(argv[1]) == "hip" &&
               gko::HipExecutor::get_num_devices() > 0) {
        exec = gko::HipExecutor::create(0, gko::OmpExecutor::create());
        if (argc == 3) index_matrix = 2;
    } else if (argc == 2) {
        exec = gko::ReferenceExecutor::create();
        index_matrix = 1;
    } else {
        std::cerr << "Usage: " << argv[0] << " [executor] [matrix]"
                  << std::endl;
        std::exit(-1);
    }
    std::cout << "index_matrix -> " << index_matrix << std::endl;

    // @sect3{Reading your data and transfer to the proper device.}
    // Read the matrix, right hand side and the initial solution using the @ref
    // read function.
    // @note Ginkgo uses C++ smart pointers to automatically manage memory. To
    // this end, we use our own object ownership transfer functions that under
    // the hood call the required smart pointer functions to manage object
    // ownership. The gko::share , gko::give and gko::lend are the functions
    // that you would need to use.
    auto A = share(gko::read<mtx>(std::ifstream("data/A.mtx"), exec));
    auto b = gko::read<vec>(std::ifstream("data/b.mtx"), exec);
    auto x = gko::read<vec>(std::ifstream("data/x0.mtx"), exec);
    if (index_matrix > 0) {
        double aux1, aux2;

        std::cout << index_matrix << std::endl;
        std::cout << index_matrix << " -> " << std::string(argv[index_matrix])
                  << std::endl;
        A = share(gko::read<mtx>(std::ifstream(argv[index_matrix]), exec));
        auto sizesA = A->get_size();
        std::cout << sizesA[0] << " - " << sizesA[1] << std::endl;
        if (std::string(argv[1]) == "cuda") {
            exec->get_master()->copy_from(exec.get(), 1, A->get_values(),
                                          &aux1);
            exec->get_master()->copy_from(
                exec.get(), 1,
                A->get_values() + A->get_num_stored_elements() - 1, &aux2);
            std::cout << "A0 => " << aux1 << " - " << aux2 << std::endl;
        } else {
            std::cout << "A0 => " << A->get_values()[0];
            std::cout << A->get_values()[A->get_num_stored_elements() - 1]
                      << std::endl;
        }

        double aux[sizesA[0]];

        //        x = gko::read<vec>(std::ifstream("data/one.mtx"), exec);
        if (std::string(argv[1]) == "cuda") {
            x = gko::read<vec>(std::ifstream("data/one.mtx"), exec);
            //            gko::kernels::cuda::zero_array(sizesA[1],
            //            b->get_values()); for (int i = 0; i<sizesA[0]; i++)
            //            aux[i] = 1.0; x->copy_from(aux);
            exec->get_master()->copy_from(exec.get(), 1, x->get_values(),
                                          &aux1);
            exec->get_master()->copy_from(
                exec.get(), 1, x->get_values() + sizesA[1] - 1, &aux2);
            std::cout << "X0 => " << aux1 << " - " << aux2 << std::endl;
        } else {
            x = vec::create(exec, gko::dim<2>{sizesA[0], 1});
            for (int i = 0; i < sizesA[0]; i++) x->at(i) = 1.0;
            std::cout << "X0 => " << x->at(0) << " - " << x->at(sizesA[0] - 1)
                      << std::endl;
        }
        auto sizesx = x->get_size();
        //        std::cout << "SIZEX -> " << sizesx[0] << " - " << sizesx[1] <<
        //        std::endl;

        //        b = gko::read<vec>(std::ifstream("data/zero.mtx"), exec);
        if (std::string(argv[1]) == "cuda") {
            b = gko::read<vec>(std::ifstream("data/zero.mtx"), exec);
            exec->get_master()->copy_from(exec.get(), 1, b->get_values(),
                                          &aux1);
            exec->get_master()->copy_from(
                exec.get(), 1, b->get_values() + sizesA[1] - 1, &aux2);
            //            std::cout << "B0 => " << aux1 << " - " << aux2 <<
            //            std::endl;
        } else {
            b = vec::create(exec, gko::dim<2>{sizesA[0], 1});
            for (int i = 0; i < sizesA[0]; i++) b->at(i) = 0.0;
            //            std::cout << "B0 => " << b->at(0) << " - " <<
            //            b->at(sizesA[0]-1) << std::endl;
        }
        auto sizesb = b->get_size();
        //        std::cout << "SIZEB -> " << sizesb[0] << " - " << sizesb[1] <<
        //        std::endl;

        A->apply(lend(x), lend(b));
        if (std::string(argv[1]) == "cuda") {
            exec->get_master()->copy_from(exec.get(), 1, b->get_values(),
                                          &aux1);
            exec->get_master()->copy_from(
                exec.get(), 1, b->get_values() + sizesA[1] - 1, &aux2);
            std::cout << "B1 => " << aux1 << " - " << aux2 << std::endl;
        } else {
            std::cout << "B1 => " << b->at(0) << " - " << b->at(sizesA[0] - 1)
                      << std::endl;
        }

        //        x = gko::read<vec>(std::ifstream("data/zero.mtx"), exec);
        if (std::string(argv[1]) == "cuda") {
            x = gko::read<vec>(std::ifstream("data/zero.mtx"), exec);
            exec->get_master()->copy_from(exec.get(), 1, x->get_values(),
                                          &aux1);
            exec->get_master()->copy_from(
                exec.get(), 1, x->get_values() + sizesA[1] - 1, &aux2);
            //            std::cout << "X0 => " << aux1 << " - " << aux2 <<
            //            std::endl;
        } else {
            x = vec::create(exec, gko::dim<2>{sizesA[0], 1});
            for (int i = 0; i < sizesA[0]; i++) x->at(i) = 0.0;
            //            std::cout << "X0 => " << x->at(0) << " - " <<
            //            x->at(sizesA[0]-1) << std::endl;
        }
    }
    // @sect3{Creating the solver}
    // Generate the gko::solver factory. Ginkgo uses the concept of Factories to
    // build solvers with certain
    // properties. Observe the Fluent interface used here. Here a gmres_mixed
    // solver is generated with a stopping criteria of maximum iterations of 20
    // and a residual norm reduction of 1e-15. You also observe that the
    // stopping criteria(gko::stop) are also generated from factories using
    // their build methods. You need to specify the executors which each of the
    // object needs to be built on.
    auto solver_gen =
        gmres_mixed::build()
            .with_criteria(
                //                gko::stop::Iteration::build().with_max_iters(20u).on(exec),
                gko::stop::Iteration::build()
                    .with_max_iters(A->get_size()[0])
                    .on(exec),
                //                gko::stop::Iteration::build().with_max_iters(100000u).on(exec),
                gko::stop::ResidualNormReduction<>::build()
                    //                    .with_reduction_factor(1e-15)
                    .with_reduction_factor(1e-12)
                    .on(exec))
            // Add preconditioner, these 2 lines are the only
            // difference from the simple solver example
            //            .with_preconditioner(bj::build().with_max_block_size(8u).on(exec))
            .with_preconditioner(bj::build().with_max_block_size(1u).on(exec))
            .on(exec);
    // Generate the solver from the matrix. The solver factory built in the
    // previous step takes a "matrix"(a gko::LinOp to be more general) as an
    // input. In this case we provide it with a full matrix that we previously
    // read, but as the solver only effectively uses the apply() method within
    // the provided "matrix" object, you can effectively create a gko::LinOp
    // class with your own apply implementation to accomplish more tasks. We
    // will see an example of how this can be done in the custom-matrix-format
    // example
    auto solver = solver_gen->generate(A);

    // Finally, solve the system. The solver, being a gko::LinOp, can be applied
    // to a right hand side, b to
    // obtain the solution, x.
    solver->apply(lend(b), lend(x));

    // Print the solution to the command line.
    //    std::cout << "Solution (x): \n";
    //    write(std::cout, lend(x));

    // To measure if your solution has actually converged, you can measure the
    // error of the solution.
    // one, neg_one are objects that represent the numbers which allow for a
    // uniform interface when computing on any device. To compute the residual,
    // all you need to do is call the apply method, which in this case is an
    // spmv and equivalent to the LAPACK z_spmv routine. Finally, you compute
    // the euclidean 2-norm with the compute_norm2 function.
    auto one = gko::initialize<vec>({1.0}, exec);
    auto neg_one = gko::initialize<vec>({-1.0}, exec);
    auto res = gko::initialize<vec>({0.0}, exec);
    A->apply(lend(one), lend(x), lend(neg_one), lend(b));
    b->compute_norm2(lend(res));

    std::cout << "Residual norm sqrt(r^T r): \n";
    write(std::cout, lend(res));
}