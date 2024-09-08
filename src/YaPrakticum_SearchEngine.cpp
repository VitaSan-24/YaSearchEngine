//============================================================================
// Name        : YaPrakticum_SearchEngine.cpp
// Author      : Vitaly Sandalov
// Version     : 0.11
// Copyright   : -
// Description : Search engine
//============================================================================

#include <iostream>
#include <utility>

using namespace std;

#include "paginator.h"
#include "document.h"
#include "request_queue.h"
#include "search_server.h"
#include "unit_test.h"

ostream& operator<<(ostream &output, const Document &document) {
    output << "{ "s << "document_id = "s << document.id << ", "s
            << "relevance = "s << document.relevance << ", "s << "rating = "s
            << document.rating << " }"s;
    return output;
}

template<typename Iterator>
std::ostream& operator<<(std::ostream &os,
        const IteratorRange<Iterator> &document_range) {

    for (Iterator it = document_range.begin(); it != document_range.end();
            ++it) {
        std::cout << *it;
    }

    return os;
}

int main() {
    TestSearchServer();
    return 0;
}
