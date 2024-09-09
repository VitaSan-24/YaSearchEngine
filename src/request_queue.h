#pragma once
/*
 * request_queue.h
 *
 *  Created on: 7 сент. 2024 г.
 *      Author: vitasan
 */
#include <string>
#include <vector>
#include <deque>
#include "search_server.h"

#include "document.h"

class RequestQueue {
public:

    explicit RequestQueue(const SearchServer &search_server) :
            search_server_(search_server) {
    }
    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template<typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string &raw_query,
            DocumentPredicate document_predicate);
    std::vector<Document> AddFindRequest(const std::string &raw_query,
            DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string &raw_query);
    int GetNoResultRequests() const;
    const SearchServer &search_server_;
private:
    struct QueryResult {
        std::vector<Document> query;
        bool request_empty;
    };
    void ProcessResultRequest(const std::vector<Document> &v_res);

    void Push(QueryResult query_result);

    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    int current_number_requests_ = 0;
    int number_empty_requests_ = 0;

};

template<typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string &raw_query,
        DocumentPredicate document_predicate) {
    const std::vector<Document> &v_res = search_server_.FindTopDocuments(
            raw_query, document_predicate);
    ProcessResultRequest(v_res);
    return v_res;
}
