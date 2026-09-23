#ifndef ____SP_TESTS_HELPER_TRIVIAL____
#define ____SP_TESTS_HELPER_TRIVIAL____

struct Trivial{
    int value;
};

struct Non_Trivial{
    int value;
    Non_Trivial() : value(0) {}
    Non_Trivial(int val) : value(val){}
    Non_Trivial(const Non_Trivial& other) : value(other.value) {}
    ~Non_Trivial() {}
};

#endif