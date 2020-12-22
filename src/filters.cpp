#include <Arduino.h>

#include "filters.h"

double filters::expRunningAverage(double toExp, double k, double filVal) {
  filVal += (toExp - filVal) * k;
  return filVal;
}

int16_t filters::median(int16_t* buf) {
  a = buf[0];
  b = buf[1];
  c = buf[2];
  if ((a <= b) && (a <= c)) {
    middle = (b <= c) ? b : c;
  } else {
    if ((b <= a) && (b <= c)) {
      middle = (a <= c) ? a : c;
    }
    else {
      middle = (a <= b) ? a : b;
    }
  }
  return middle;
}

// double filters::exp_mid(double toEM, double k){
//   return median(expRunningAverage(toEM, k));
// }

// double filters::mid_exp(double toME, double k){
//   return expRunningAverage(median(toME), k);
// }