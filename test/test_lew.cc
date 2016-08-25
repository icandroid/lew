
#include    "src/gtest-all.cc"

#include    "test_tcp_http.cc"

static  int
_run_all_tests(int  argc, char* argv[]){
    ::testing::InitGoogleTest(&argc, argv);
    int     ret = RUN_ALL_TESTS();
    return  (ret);
}

int     main(int    argc,   char*   argv[]){
    int     ret = _run_all_tests(argc, argv);
    return  (ret);
}

