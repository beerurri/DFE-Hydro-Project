#ifndef FILTERS_H
#define FILTERS_H

class filters{
  public:
    int16_t median(int16_t* buf);
    double expRunningAverage(double toExp, double k, double filVal);
    // double exp_mid(double toEM, double k);
    // double mid_exp(double toME, double k);
  
  private:
    int16_t* buf;
    int16_t a;
    int16_t b;
    int16_t c;
    int16_t middle;
    String _id;

    //double k;
    double filVal;

};

#endif
