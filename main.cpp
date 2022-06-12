#include "search_server.h"
#include <iomanip>

template <typename Func>
void RunTestImpl(Func f, const std::string& s) {
    f();
    std::cerr << s << " OK" << std::endl;
}

void AssertImpl(bool expression,
    const std::string& str,
    const std::string& file,
    const std::string& function,
    const unsigned line,
    const std::string& hint) {

    if (!expression) {
        std::cerr << file << "(" << line << "): " << function << ": ASSERT(" << str << ") failed. ";
        if (!hint.empty()) {
            std::cerr<< "Hint: " << hint;
        }
        std::cerr << std::endl;
        abort();
    }
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
    const std::string& func, unsigned line, const std::string& hint) {
    if (t != u) {
        std::cerr << std::boolalpha;
        std::cerr << file << "(" << line << "): " << func << ": ";
        std::cerr << "ASSERT_EQUAL(" << t_str << ", " << u_str << ") failed: ";
        std::cerr << t << " != " << u << ".";
        if (!hint.empty()) {
            std::cerr << " Hint: " << hint;
        }
        std::cerr << std::endl;
        abort();
    }
}

//macros for testing
#define ASSERT_HINT(expr, hint) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__,  hint)
#define ASSERT(expr)  AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__,  "")

#define ASSERT_EQUAL_HINT(left, right, hint) AssertEqualImpl((left), (right), #left, #right, __FILE__, __FUNCTION__, __LINE__, hint)
#define ASSERT_EQUAL(left, right) ASSERT_EQUAL_HINT(left, right, "");

#define RUN_TEST(func)  RunTestImpl((func), #func)

//function for comparing floating-point numbers



// The TestSearchServer function is the entry point for running tests
void TestSearchServer() {

    RUN_TEST(TestStopWords);

    RUN_TEST(SearchServer_AddDocument_CheckSize_SizeChange);
    RUN_TEST(SearchServer_AddDocument_CheckSize_SizeEmpty);
    RUN_TEST(SearchServer_AddDocument_CheckId_IdFound);
    RUN_TEST(SearchServer_AddDocument_CheckDocumentsCount_Equal);

    RUN_TEST(TestMinusWords);
    RUN_TEST(TestMatchingDocuments);
    RUN_TEST(TestByRelevance);
    RUN_TEST(TestByRelevance_MinusWords);
    RUN_TEST(TestOfRating);
    RUN_TEST(TestSortingByPredicate);

    RUN_TEST(SearchServer_Status_CheckSize_SizeChange);
    RUN_TEST(SearchServer_Status_CheckSize_SizeEmpty);
    RUN_TEST(SearchServer_Status_CheckId_IdFound);
    RUN_TEST(SearchServer_Status_CheckDocumentsCount_Equal);
}

// --------- End of search engine unit tests -----------

int main() {
    TestSearchServer();
    std::cout << "Search server testing finished" << std::endl;
}