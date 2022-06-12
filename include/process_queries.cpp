#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server, const std::vector<std::string>& queries) {
	std::vector<std::vector<Document>> result(queries.size());

	std::transform(std::execution::par, queries.begin(), queries.end(), result.begin(), [&search_server](const std::string& request) {
		return search_server.FindTopDocuments(request);
		});

	return result;
}

std::vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries) {

	std::vector<Document> result;
	for (auto v : ProcessQueries(search_server, queries)) {
		std::copy(v.begin(), v.end(), std::back_inserter(result));
	}
	return result;
}