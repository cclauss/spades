#!/bin/bash

# compile and install libraries from ext

#export assembler="`pwd`\.."
#export src="$assembler/src"
#export ext="$assembler/ext"
#export build="$assembler/build"

function print_heading 
{
   echo "/**************************************/"
   echo "          $1                            "
   echo "/**************************************/"
}

function build_fftw
{
   print_heading 'Building fftw'
   
   mkdir -p $build/ext/fftw
   cd $build/ext/fftw
   make distclean
   
   $ext/src/fftw-3.3/configure --prefix="`pwd`"

   make
   make install
}

function build_sparsehash
{
   print_heading 'Building sparsehash'
   
   mkdir -p $build/ext/sparsehash
   cd $build/ext/sparsehash
   make distclean
   
   $ext/src/google-sparsehash-read-only/configure --prefix="`pwd`"

   make
   make install
}

build_fftw
build_sparsehash
  
