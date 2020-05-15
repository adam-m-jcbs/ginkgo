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

#ifndef GKO_CORE_SOLVER_BICGSTAB_HPP_
#define GKO_CORE_SOLVER_BICGSTAB_HPP_


#include <vector>


#include <ginkgo/core/base/array.hpp>
#include <ginkgo/core/base/exception_helpers.hpp>
#include <ginkgo/core/base/lin_op.hpp>
#include <ginkgo/core/base/math.hpp>
#include <ginkgo/core/base/types.hpp>
#include <ginkgo/core/log/logger.hpp>
#include <ginkgo/core/matrix/identity.hpp>
#include <ginkgo/core/stop/combined.hpp>
#include <ginkgo/core/stop/criterion.hpp>


namespace gko {
/**
 * @brief The ginkgo Solve namespace.
 *
 * @ingroup solvers
 */
namespace solver {


/**
 * BiCGSTAB or the Bi-Conjugate Gradient-Stabilized is a Krylov subspace solver.
 *
 * Being a generic solver, it is capable of solving general matrices, including
 * non-s.p.d matrices. Though, the memory and the computational requirement of
 * the BiCGSTAB solver are higher than of its s.p.d solver counterpart, it has
 * the capability to solve generic systems. It was developed by stabilizing the
 * BiCG method.
 *
 * @tparam ValueType precision of the elements of the system matrix.
 *
 * @ingroup bicgstab
 * @ingroup solvers
 * @ingroup LinOp
 */
template <typename ValueType = default_precision>
class Bicgstab : public EnableLinOp<Bicgstab<ValueType>>,
                 public Preconditionable,
                 public Transposable {
    friend class EnableLinOp<Bicgstab>;
    friend class EnablePolymorphicObject<Bicgstab, LinOp>;

public:
    using value_type = ValueType;
    using transposed_type = Bicgstab<ValueType>;

    /**
     * Gets the system operator (matrix) of the linear system.
     *
     * @return the system operator (matrix)
     */
    std::shared_ptr<const LinOp> get_system_matrix() const
    {
        return system_matrix_;
    }

    std::unique_ptr<LinOp> transpose() const override;

    std::unique_ptr<LinOp> conj_transpose() const override;

    /**
     * Return true as iterative solvers use the data in x as an initial guess.
     *
     * @return true as iterative solvers use the data in x as an initial guess.
     */
    bool apply_uses_initial_guess() const override { return true; }

    GKO_CREATE_FACTORY_PARAMETERS(parameters, Factory)
    {
        /**
         * Criterion factories.
         */
        std::vector<std::shared_ptr<const stop::CriterionFactory>>
            GKO_FACTORY_PARAMETER(criteria, nullptr);

        /**
         * Preconditioner factory.
         */
        std::shared_ptr<const LinOpFactory> GKO_FACTORY_PARAMETER(
            preconditioner, nullptr);

        /**
         * Already generated preconditioner. If one is provided, the factory
         * `preconditioner` will be ignored.
         */
        std::shared_ptr<const LinOp> GKO_FACTORY_PARAMETER(
            generated_preconditioner, nullptr);
    };
    GKO_ENABLE_LIN_OP_FACTORY(Bicgstab, parameters, Factory);
    GKO_ENABLE_BUILD_METHOD(Factory);

protected:
    void apply_impl(const LinOp *b, LinOp *x) const override;

    void apply_impl(const LinOp *alpha, const LinOp *b, const LinOp *beta,
                    LinOp *x) const override;

    explicit Bicgstab(std::shared_ptr<const Executor> exec)
        : EnableLinOp<Bicgstab>(std::move(exec))
    {}

    explicit Bicgstab(const Factory *factory,
                      std::shared_ptr<const LinOp> system_matrix)
        : EnableLinOp<Bicgstab>(factory->get_executor(),
                                gko::transpose(system_matrix->get_size())),
          parameters_{factory->get_parameters()},
          system_matrix_{std::move(system_matrix)}
    {
        if (parameters_.generated_preconditioner) {
            GKO_ASSERT_EQUAL_DIMENSIONS(parameters_.generated_preconditioner,
                                        this);
            set_preconditioner(parameters_.generated_preconditioner);
        } else if (parameters_.preconditioner) {
            set_preconditioner(
                parameters_.preconditioner->generate(system_matrix_));
        } else {
            set_preconditioner(matrix::Identity<ValueType>::create(
                this->get_executor(), this->get_size()[0]));
        }
        stop_criterion_factory_ =
            stop::combine(std::move(parameters_.criteria));
    }

private:
    std::shared_ptr<const LinOp> system_matrix_{};
    std::shared_ptr<const stop::CriterionFactory> stop_criterion_factory_{};
};


}  // namespace solver
}  // namespace gko


#endif  // GKO_CORE_SOLVER_BICGSTAB_HPP_
