#! /bin/bash

for n in cgs mks num
do
  emacs -batch -l const.el -f run-$n > gsl_const_$n.h
  ( cd .. ;  ./scripts/addgpl.pl "Brian Gough" const/gsl_const_$n.h )
done 