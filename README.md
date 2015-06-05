pgibbs - Parallelized Unsupervised word segmentation and POS tagging
====================================================================

by [Graham Neubig](http://www.phontron.com)

This package implements parallel Gibbs sampling for word segmentation and POS tagging using both blocked and multi-sampler methods. The two executables are src/bin/pgibbs-ws for word segmentation and src/bin/pgibbs-hmm for POS tagging.
You can find more details in the following paper:

"[Simple, Correct Parallelization for Blocked Gibbs Sampling](http://www.phontron.com/paper/neubig14pgibbs.pdf)". Graham Neubig. Technical Report. 2014.

    @techreport{neubig14pgibbs,
      author = {Graham Neubig},
      title = {Simple, Correct Parallelization for Blocked Gibbs Sampling},
      institution = {Nara Institute of Science and Technology},
      year = {2014},
      url = {http://www.phontron.com/paper/neubig14pgibbs.pdf}
    }

Installation
------------

To compile pgibbs, download it from git, and run

    autoreconf -i
    ./configure
    make

Data Preparation
----------------

For HMM-based tagging, the input is divided into words:

上海 浦东 开发 与 法制 建设 同步

And for word-segmentation, the input is divided into characters:

上 海 浦 东 开 发 与 法 制 建 设 同 步

If you want to replicate experiments in the technical report, you must have the Chinese Treebank 5.0, which can be obtained from the LDC (details [here](http://www.ldc.upenn.edu/Catalog/CatalogEntry.jsp?catalogId=LDC2005T01)).

Running Programs
----------------

Both programs can be run with

    pgibbs-{ws,hmm} [OPTIONS] INPUTDATA OUTPUTPREFIX

The input data is the previously mentioned files, and the output data is a prefix where the label files will be written. The major options used are as follows:

    -iters 2000             // The number of iterations
    -threads {1,2,4,8}      // The number of threads to use
    -blocksize {1,2,4,10,20,40,100} // The size of a single block for blocked sampling
    -sampmeth {block,parallel} // Whether to perform blocked or parallel sampling
    -skipiters {0,2000}     // Will skip a Metropolis-Hastings rejection for a certain
                            //  number of iterations, when set equal to the number of
                            //  iterations, MH will not be performed

One thing to note here is that if you are using "-sampmeth block", "-blocksize" should be larger than "-threads", maybe 4 times larger (see the paper for details).

There are also additional options for word segmentation only:

    -n 2                    // The n-gram size of the model

Or for the HMM only:

    -classes 30             // The number of classes in the model

Execution Examples
------------------

An example of running the program for part-of-speech tagging for the file test.words on 4 cores is below:

    pgibbs-hmm -iters 500 -threads 4 -blocksize 16 -sampmeth block -skipiters 500 test.words output-hmm

An example of running the program for word segmentation on the file test.chars on 4 cores is below:

    pgibbs-ws  -iters 500 -threads 4 -blocksize 16 -sampmeth block -skipiters 0   test.chars output-ws
