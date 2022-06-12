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
            std::cerr << "Hint: " << hint;
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

#define TEST TestSearchServer();

bool is_equal(const double l, const double r) {
    const double EPSILON = 1e-6;
    return (std::abs(l - r) < EPSILON);
}

// -------- Start of search engine unit tests ----------

//creating an instance of a search server that covers all the tests
SearchServer GetTestServer() {

    SearchServer server("1word1 2word2 3word3"s);

    server.AddDocument(0, "1word1 1word2 1word3 1word4", DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(1, "2word1 2word2 2word3 2word4", DocumentStatus::BANNED, { 4, 5, 6, 7, 8 });
    server.AddDocument(2, "3word1 3word2 3word3 3word4 3word3 3word4", DocumentStatus::IRRELEVANT, { 1, 3, 4, 5, 6, 7, 8 });
    server.AddDocument(3, "4word1 4word2 4word3 4word4", DocumentStatus::REMOVED, { 4, 5, 6, 7, 8, 20, 9 });
    server.AddDocument(4, "5word1 5word2 5word3 5word4 5word3 5word4", DocumentStatus::ACTUAL, { 5, 1, 3, 4, 5, 6, 7, 8 });
    server.AddDocument(5, "6word1 6word2 6word1 6word2", DocumentStatus::ACTUAL, { 9, 4, 5, 6, 7, 8, 20, 9 });

    return server;
}

void SearchServer_AddDocument_CheckSize_SizeChange() {

    //empty server
    SearchServer server(""s);

    //search for a non-existent word
    std::vector<Document> fd = server.FindTopDocuments("1word1");
    ASSERT_HINT(fd.empty(), "word doesn't exist");

    //fill server by data
    server = GetTestServer();

    fd = server.FindTopDocuments("1word2");

    ASSERT(fd.size() == 1);
    ASSERT_EQUAL_HINT(server.GetDocumentCount(), 6, "Documents count == 6");
}

void SearchServer_AddDocument_CheckSize_SizeEmpty() {
    SearchServer server = GetTestServer();

    const std::vector<Document>& fd = server.FindTopDocuments("2word2", DocumentStatus::BANNED);

    ASSERT(fd.empty());
}

void SearchServer_AddDocument_CheckId_IdFound() {
    SearchServer server = GetTestServer();

    const std::vector<Document>& fd = server.FindTopDocuments("1word2");

    ASSERT_EQUAL(fd[0].id, 0);
}

void SearchServer_AddDocument_CheckDocumentsCount_Equal() {
    SearchServer server(""s);

    std::vector<Document> fd = server.FindTopDocuments("1word2");

    ASSERT_EQUAL(fd.size(), 0);

    server.AddDocument(0, "1word2", DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(1, "2word2", DocumentStatus::ACTUAL, { 1, 2, 3 });

    fd = server.FindTopDocuments("1word2 2word2");

    ASSERT_EQUAL(fd.size(), 2);
}

void TestStopWords() {
    SearchServer server = GetTestServer();

    const std::vector<Document>& fd = server.FindTopDocuments("2word2");

    ASSERT_HINT(fd.empty(), "vector must be empty");
}

void TestMinusWords() {
    SearchServer server = GetTestServer();

    const std::vector<Document>& fd1 = server.FindTopDocuments("1word2 -1word3");

    ASSERT(fd1.empty());

    const std::vector<Document>& fd2 = server.FindTopDocuments("2word1 -1word3", DocumentStatus::BANNED);

    ASSERT_EQUAL(fd2[0].id, 1);

}

void TestMatchingDocuments() {
    SearchServer server = GetTestServer();

    const auto& [w1, ds1] = server.MatchDocument("1word1 1word2 2word1 2word2", 0);
    const auto& [w2, ds2] = server.MatchDocument("1word1 1word2 2word1 2word2", 1);
    const auto& [w3, ds3] = server.MatchDocument("1word1 1word2 -2word1 2word2", 0);
    const auto& [w4, ds4] = server.MatchDocument("-1word2 -2word1 2word2", 0);

    ASSERT(w1[0] == "1word2"); ASSERT(ds1 == DocumentStatus::ACTUAL);
    ASSERT(w2[0] == "2word1"); ASSERT(ds2 == DocumentStatus::BANNED);
    ASSERT(w3[0] == "1word2"); ASSERT(ds3 == DocumentStatus::ACTUAL);
    ASSERT(w4.empty());

}

void TestByRelevance() {
    SearchServer server = GetTestServer();

    const std::vector<Document>& fd1 = server.FindTopDocuments("1word2 2word1 3word1 3word2 3word3 4word1 5word5 5word2 6word3 6word1");

    ASSERT(!is_equal(fd1[1].relevance, fd1[0].relevance));
    ASSERT(!is_equal(fd1[2].relevance, fd1[1].relevance));
}

void TestByRelevance_MinusWords() {
    SearchServer server = GetTestServer();

    const std::vector<Document>& fd2 = server.FindTopDocuments("-1word2 -2word1 3word1 3word2 3word3 4word1 5word5 5word2 6word3 6word1");

    ASSERT_EQUAL(is_equal(fd2[0].relevance, 0.895879), true);
    ASSERT_EQUAL(is_equal(fd2[1].relevance, 0.298626), true);
}

void TestOfRating() {
    SearchServer server = GetTestServer();

    const std::vector<Document>& fd = server.FindTopDocuments("-1word2 -2word1 3word1 3word2 3word3 4word1 5word5 5word2 6word3 6word1");

    ASSERT_EQUAL(fd[0].rating, 8);
    ASSERT_HINT(fd[1].rating < fd[0].rating, "");
}

void TestSortingByPredicate() {
    SearchServer server = GetTestServer();

    //search for documents with the condition: rating no more than 8 and no less than 2
    const std::vector<Document>& fdsp = server.FindTopDocuments("-1word2 -2word1 3word1 3word2 3word3 4word1 5word5 5word2 6word3 6word1",
        [](const int id, const DocumentStatus ds, int rating) {return rating < 8 && 2 < rating; });

    ASSERT_EQUAL_HINT(fdsp.size(), 2, "docs with rating 4");
}

void SearchServer_Status_CheckSize_SizeChange() {

    SearchServer server = GetTestServer();

    const std::vector<Document>& fdA = server.FindTopDocuments("-1word2 -2word1 3word1 3word2 3word3 4word1 5word5 5word2 6word3 6word1", DocumentStatus::ACTUAL);
    const std::vector<Document>& fdB = server.FindTopDocuments("1word2 2word1 3word1 3word2 3word3 4word1 5word5 5word2 6word3 6word1", DocumentStatus::IRRELEVANT);

    ASSERT_EQUAL(fdA.size(), 2);
    ASSERT_EQUAL(fdB.size(), 1);
}

void SearchServer_Status_CheckSize_SizeEmpty() {

    SearchServer server = GetTestServer();

    const std::vector<Document>& fd = server.FindTopDocuments("-1word2 -2word1 3word1 3word2 3word3 4word1 5word5 5word2 6word3 6word1", DocumentStatus::BANNED);

    ASSERT(fd.empty());
}

void SearchServer_Status_CheckId_IdFound() {

    SearchServer server = GetTestServer();

    const std::vector<Document>& fd = server.FindTopDocuments("-1word2 -2word1 3word1 3word2 3word3 4word1 5word5 5word2 6word3 6word1", DocumentStatus::REMOVED);

    ASSERT_EQUAL(fd[0].id, 3);
}

void SearchServer_Status_CheckDocumentsCount_Equal() {

    SearchServer server = GetTestServer();

    const std::vector<Document>& fd = server.FindTopDocuments("-1word2 -2word1 3word1 3word2 3word3 4word1 5word5 5word2 6word3 6word1", DocumentStatus::IRRELEVANT);

    ASSERT_EQUAL(fd.size(), 1);
}

void TestIterators() {
    SearchServer server = GetTestServer();
    int i = 0;
    for (const int d : server) {
        ASSERT_EQUAL(d, i++);
    }
}

void TestGetWordFrequencies() {
    SearchServer server = GetTestServer();
    const std::map<std::string, double>& result1 = server.GetWordFrequencies(1);
    const std::map<std::string, double>& result5 = server.GetWordFrequencies(5);
    const std::map<std::string, double>& result8 = server.GetWordFrequencies(8);

    ASSERT_EQUAL(result1.size(), 3);

    ASSERT_EQUAL(is_equal(result1.at("2word1"), 0.33333333333333331), true);

    ASSERT_EQUAL(is_equal(result5.at("6word1"), 0.5), true);
    ASSERT_EQUAL(is_equal(result5.at("6word2"), 0.6), false);   

    ASSERT_EQUAL(result8.size(), 0);
}

void TestRemoveDocuments() {
    SearchServer server = GetTestServer();
    server.RemoveDocument(3);
    int x = 2;
    //ASSERT_EQUAL(server.)
}

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

    RUN_TEST(TestIterators);
    RUN_TEST(TestGetWordFrequencies);
    RUN_TEST(TestRemoveDocuments);

}