---
title: 'Ginkgo: A high performance numerical linear algebra library'
tags:
  - linear-algebra
  - hpc
  - cuda
  - modern-c++
  - hip
  - spmv
authors:
  - name: Hartwig Anzt
    orcid: 0000-0003-2177-952X
    affiliation: "1, 2" # (Multiple affiliations must be quoted)
  - name: Terry Cojean
    orcid: 0000-0002-1560-921X
    affiliation: 1 
  - name: Yen-Chen Chen 
    affiliation: 4 
  - name: Goran Flegar
    orcid: 0000-0002-4154-0420
    affiliation: 3 
  - name: Fritz G{\"o}bel
    affiliation: 1 
  - name: Thomas Gr{\"u}tzmacher 
    affiliation: 1
  - name: Pratik Nayak 
    orcid: 0000-0002-7961-1159
    affiliation: 1 
  - name: Tobias Ribizel 
    affiliation: 1 
	orcid: 0000-0003-3023-1849
  - name: Yu-Hsiang Tsai 
    orcid: 0000-0001-5229-3739
    affiliation: 1 
affiliations:
 - name: Karlsruhe Institute of Technology
   index: 1
 - name: Innovative Computing Laboratory, University of Tennessee, Knoxville 
   index: 2
 - name: University of Jaume I 
   index: 3
 - name: The University of Tokyo 
   index: 4
date: 6th March, 2020.
bibliography: paper.bib

---

# Summary

Ginkgo is a production-ready sparse linear (operator) algebra library for high
performance computing on GPU-centric architectures with a high level of
performance portability and focuses on software sustainability. 

The library focuses on solving sparse linear systems and accommodates a large variety
of matrix formats, state-of-the-art iterative (Krylov) solvers and preconditioners, 
which make the library suitable for a variety of scientific applications. Ginkgo
supports many architectures such as multi-threaded CPU, NVIDIA GPUs, and AMD GPUs.
The heavy use of modern C++ 11 features simplifies the addition of new executor
paradigms and algorithmic functionality without introducing significant
performance overhead.

Ginkgo is also a part of the xSDK effort [@xsdk] and available as a Spack
[@spack] package. xSDK aims to provide infrastructure for and interoperability
between a collection of related and complementary software elements to foster
rapid and efficient development of scientific applications using High
Performance Computing. Within this effort, we provide interoperability with
application libraries such as `deal.ii`[@dealii] and `mfem`[@mfem]. Ginkgo
provides wrappers within these two libraries so that they can take advantage of
the features of Ginkgo.

# Features

As sparse linear algebra is one of the main focus of Ginkgo, we provide a variety of 
sparse matrix formats such as COO, CSR, ELL, HYBRID and SELLP along with highly tuned
Sparse Matrix Vector product (SpMV) kernels. The SpMV kernel is a key building 
blocks of vitually all iterative solvers and typically accounts for a significant 
fraction of the application runtime. Additionally, we also provide high performance 
conversion routines between the different formats enhancing their flexibility.

Ginkgo provides multiple iterative solvers such as the Krylov subspace
methods: Conjugate gradient (CG), Flexible Conjugate Gradient (FCG), Bi-Conjugate
Gradient (BiCG) and its stabilized version (Bi-CGSTAB), Generalized Minimal
residual method (GMRES) and more generic methods such as Iterative Refinement,
which forms the basis of many relaxation methods. Ginkgo also features support for 
direct and iterative triangular solves within incomplete factorization preconditioners.

Ginkgo features some of the best preconditioners such as the general-purpose Block Jacobi
preconditioner with support for a version which reduces pressure on the memory bandwidth 
by dynamically adapting the memory precision to the numerical requirements.
This [@adaptive-bj] has been shown to be very efficient for problems with a block 
structure.
Ginkgo also features highly-parallel incomplete factorization preconditioners such as 
the ParILU and the ParILUT preconditioners [@parilut].


# Software extensibility and sustainability.

Ginkgo is extensible in terms of linear algebra solvers, preconditioners and
matrix formats. Basing on modern C++ (C++11 standard), various language features
such as data abstraction, generic programming and automatic memory management are
leveraged to enhance the performance of the library while still maintaining ease of 
use and maintenance. 

The Ginkgo library is constructed around two principal design concepts. The first
one is the class and object-oriented design based in part on linear operators
which aims to provide an easy to use interface, common for all the devices and
linear algebra objects. This allows users to easily cascade solvers,
preconditioners or matrix formats and tailor solvers for their needs in a
seamless fashion. The second main design concept consists of the low level
device specific kernels. These low level kernels are optimized for the specific
device and make use of C++ features such as templates to generate
high-performance kernels for a wide variety of parameters. 



![Core architecture of Ginkgo. All solvers, preconditioners and matrix formats
are accessible through the same LinOp interface.](figures/ginkgo-hierarchy.png)

Ginkgo adopts a rigorous approach to testing and maintenance. Using continuous
integration tools such as Gitlab-CI and Github-Actions, we make sure that 
the library builds on a range of compilers and environments. To verify that our code
quality is of the highest standards we use the Sonarcloud platform [@sonarcloud],
a code analyzer to analyze and report any code smells. To ensure correctness of
the code, we use the Google Test library [@gtest] to rigorously test each of the
device specific kernels and the core framework of the library. 


# Performance and Benchmarking

The Ginkgo software is tailored for High Performance Computing and provides high
performance implementation on modern manycore architectures. In particular,
Ginkgo is competitive with hardware vendor libraries such as hipSPARSE and
cuSPARSE [@2019spmvhip]. 

![Ginkgo Hybrid spmv performance compared against (left) cuSPARSE and (right)
hipSPARSE](figures/ginkgo-hybrid.png) 


![Ginkgo CSR spmv performance compared against (left) cuSPARSE and (b)
hipSPARSE](figures/ginkgo-csr.png) 

Ginkgo provides comprehensive logging facilities both in-house and with interfaces
to external libraries such as PAPI [@papi]. This allows for detailed analysis of
the kernels while reducing the intellectual overhead of optimizing the applications.

To enhance reproducibility from a performance perspective, we provide the performance 
results of our kernel implementations in an open source git repository [@gko-data].

A unique feature of Ginkgo is the availability of an interactive webtool, the Ginkgo
Performance explorer [@gpe], which can plot results from the aforementioned data 
repository. Additionally, we have also put in some effort in making benchmarking
easier, within the Ginkgo repository using the `rapidjson` [@rapidjson] and 
`gflags` [@gflags] libraries to run and generate benchmarking results for a variety 
of Ginkgo features.

# Acknowledgements
Research and software development in Ginkgo received support from the Helmholtz 
association (Impuls und Vernetzungsfond VH-NG-1241), and the US Exascale Computing 
Project (17-SC-20-SC), a collaborative effort of the U.S. Department of Energy Office 
of Science and the National Nuclear Security Administration.

# References