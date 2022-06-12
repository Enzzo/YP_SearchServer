#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {

    std::set<int> duplicates;
    std::map<std::set<std::string>, int> temp;

    for (const int id : search_server) {
        const std::map<std::string, double>& doc = search_server.GetWordFrequencies(id);
        std::set<std::string> content;
        
        std::transform(doc.begin(), doc.end(), std::inserter(content, content.begin()), [](const std::pair<std::string, double>& d) {
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