/*
 * request_queue.cpp
 *
 *  Created on: 7 сент. 2024 г.
 *      Author: vitasan
 */

#include "request_queue.h"
#include "search_server.h"

std::vector<Document> RequestQueue::AddFindRequest(const std::string &raw_query,
        DocumentStatus status) {
    const std::vector<Document> &v_res = search_server_.FindTopDocuments(
            raw_query, status);
    ProcessResultRequest(v_res);
    return v_res;
}
std::vector<Document> RequestQueue::AddFindRequest(
        const std::string &raw_query) {
    const std::vector<Document> &v_res = search_server_.FindTopDocuments(
            raw_query);
    ProcessResultRequest(v_res);
    return v_res;
}
int RequestQueue::GetNoResultRequests() const {
    return number_empty_requests_;
}

void RequestQueue::ProcessResultRequest(const std::vector<Document> &v_res) {
    QueryResult query_result;
    if (v_res.empty()) {
        query_result = { v_res, true };
    } else {
        query_result = { v_res, false };
    }
    Push(query_result);
}

void RequestQueue::Push(QueryResult query_result) {
    if (min_in_day_ > current_number_requests_) {
        requests_.push_back(query_result); // @suppress("Invalid arguments")
        ++current_number_requests_; // добавляем количество минут
    } else {
        requests_.push_back(query_result); // @suppress("Invalid arguments")
        if (requests_.front().request_empty) // @suppress("Field cannot be resolved")
            --number_empty_requests_; // Уменьшаем количество пустых слов
        requests_.pop_front();
    }
    if (query_result.request_empty)
        ++number_empty_requests_; // Увеличиваем количество пустых запросов

}

