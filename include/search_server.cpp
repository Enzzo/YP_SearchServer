#include "search_server.h"

SearchServer::SearchServer(const std::string_view& stop_words_text) : SearchServer(SplitIntoWords(stop_words_text)) {}

SearchServer::SearchServer(const std::string& stop_words_text) : SearchServer(SplitIntoWords(stop_words_text)) {}

void SearchServer::AddDocument(int document_id, const std::string_view& document, DocumentStatus status,
    const std::vector<int>& ratings) {

    if ((document_id < 0)) {
        throw std::invalid_argument("ID can't be less than zero"s);
    }
    if (documents_.count(document_id) > 0) {
        throw std::invalid_argument("ID already exists"s);
    }

    std::vector<std::string> words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();
    for (const std::string& word : words) {
        doc_to_word_freqs_[document_id][word] += inv_word_count;
        word_to_doc_freqs_[word][document_id] += inv_word_count;
    }

    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_id_.emplace(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query, DocumentStatus status) const {

    return FindTopDocuments(
        raw_query,
        [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view& raw_query, int document_id) const {
    return MatchDocument(std::execution::seq, raw_query, document_id);
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(const int document_id) const noexcept {

    auto it = doc_to_word_freqs_.find(document_id);

    std::map<std::string_view, double>* result = new std::map<std::string_view, double>;

    if (it != doc_to_word_freqs_.end()) {
        result->insert(it->second.begin(), it->second.end());
        /*
        for (auto& [word, freq] : it->second) {
            result.emplace(std::string_view(word), freq);
        }
        */
    }
    return *result;
}

void SearchServer::RemoveDocument(const int document_id) {
    RemoveDocument(std::execution::seq, document_id);
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = 0;
    for (const int rating : ratings) {
        rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string_view& text) const {
    std::vector<std::string> result;

    for (const std::string& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            return {};
        }
        if (!IsStopWord(word)) {
            result.push_back(word);
        }
    }
    return result;
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const {

    if (text.empty()) {
        return QueryWord();
    }
    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    if (text.empty() || text[0] == '-' || !IsValidWord(text)) {
        return QueryWord();
    }


    return QueryWord{ text, is_minus, IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view& text) const {

    Query result;

    for (const std::string& word : SplitIntoWords(text)) {
        QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.insert(query_word.data);
            }
            else {
                result.plus_words.insert(query_word.data);
            }
        }
    }
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    int size = 0;

    for (const auto& [doc, content] : doc_to_word_freqs_) {
        if (content.count(word)) {
            size++;
        }
    }

    return log(GetDocumentCount() * 1.0 / size);
}

void RemoveDuplicates(SearchServer& search_server) {

    std::set<int> duplicates;
    std::map<std::set<std::string_view>, int> temp;

    for (const int id : search_server) {
        const std::map<std::string_view, double>& doc = search_server.GetWordFrequencies(id);
        std::set<std::string_view> content;

        std::transform(doc.begin(), doc.end(), std::inserter(content, content.begin()), [](const std::pair<std::string_view, double>& d) {
            return d.first;
            });

        if (temp.count(content)) {
            std::cout << "Found duplicate document id " << id << "\n";
            duplicates.emplace(id);
        }
        else {
            temp.emplace(content, id);
        }
    }

    for (const int id : duplicates) {
        search_server.RemoveDocument(id);
    }
}

