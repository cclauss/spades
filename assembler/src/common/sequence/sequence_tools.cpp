#include "edlib/edlib.h"
#include "sequence_tools.hpp"

static edlib::EdlibEqualityPair additionalEqualities[36] = {{'U', 'T'}
    , {'R', 'A'}, {'R', 'G'}
    , {'Y', 'C'}, {'Y', 'T'}, {'Y', 'U'}
    , {'K', 'G'}, {'K', 'T'}, {'K', 'U'}
    , {'M', 'A'}, {'M', 'C'}
    , {'S', 'C'}, {'S', 'G'}
    , {'W', 'A'}, {'W', 'T'}, {'W', 'U'}
    , {'B', 'C'}, {'B', 'G'}, {'B', 'T'}, {'B', 'U'}
    , {'D', 'A'}, {'D', 'G'}, {'D', 'T'}, {'D', 'U'}
    , {'H', 'A'}, {'H', 'C'}, {'H', 'T'}, {'H', 'U'}
    , {'V', 'A'}, {'V', 'C'}, {'V', 'G'}
    , {'N', 'A'}, {'N', 'C'}, {'N', 'G'}, {'N', 'T'}, {'N', 'U'}
};

int StringDistance(const std::string &a, const std::string &b, int max_score) {
    int a_len = (int) a.length();
    int b_len = (int) b.length();
    int d = std::min(a_len / 3, b_len / 3);
    d = std::max(d, 10);
    if (a_len == 0 || b_len == 0) {
        if (d > 10) {
            // TRACE("zero length path , lengths " << a_len << " and " << b_len);
            return std::numeric_limits<int>::max();
        } else {
            return std::max(a_len, b_len);
        }
    }
    if (max_score == -1) {
        max_score = 2 * d;
    }
    // DEBUG(a_len << " " << b_len << " " << d);

    edlib::EdlibAlignResult result = edlib::edlibAlign(a.c_str(), a_len,
                                     b.c_str(), b_len,
                                     edlib::edlibNewAlignConfig(max_score, edlib::EDLIB_MODE_NW, edlib::EDLIB_TASK_DISTANCE,
                                             additionalEqualities, 36));
    int score = std::numeric_limits<int>::max();
    if (result.status == edlib::EDLIB_STATUS_OK && result.editDistance >= 0) {
        score = result.editDistance;
    }
    edlib::edlibFreeAlignResult(result);
    return score;
}


void SHWDistanceFull(const std::string &target, const std::string &query, int max_score, std::vector<int> &positions, std::vector<int> &scores) {
    if (query.size() == 0) {
        for (int i = 0; i < std::min(max_score, (int) target.size()); ++ i) {
            positions.push_back(i);
            scores.push_back(i + 1);
        }
        return;
    }
    if (target.size() == 0) {
        if (query.size() <= max_score) {
            positions.push_back(0);
            scores.push_back(query.size());
        }
        return;
    }
    VERIFY(target.size() > 0)
    edlib::EdlibAlignResult result = edlib::edlibAlign(query.c_str(), (int) query.size(), target.c_str(), (int) target.size()
                                     , edlib::edlibNewAlignConfig(max_score, edlib::EDLIB_MODE_SHW_FULL, edlib::EDLIB_TASK_DISTANCE,
                                             additionalEqualities, 36));
    if (result.status == edlib::EDLIB_STATUS_OK && result.editDistance >= 0) {
        positions.reserve(result.numLocations);
        scores.reserve(result.numLocations);
        for (int i = 0; i < result.numLocations; ++ i) {
            //INFO("Loc=" << result.endLocations[i] << " score=" << result.endScores[i]);
            if (result.endLocations[i] >= 0) {
                positions.push_back(result.endLocations[i]);
                scores.push_back(result.endScores[i]);
            }
        }
    }
    edlib::edlibFreeAlignResult(result);
}

int SHWDistance(const std::string &a, const std::string &b, int max_score, int &end_pos) {
    int a_len = (int) a.length();
    int b_len = (int) b.length();
    VERIFY(a_len > 0);
    VERIFY(b_len > 0);
    edlib::EdlibAlignResult result = edlib::edlibAlign(a.c_str(), a_len, b.c_str(), b_len
                                     , edlib::edlibNewAlignConfig(max_score, edlib::EDLIB_MODE_SHW, edlib::EDLIB_TASK_DISTANCE,
                                             additionalEqualities, 36));
    int score = std::numeric_limits<int>::max();
    if (result.status == edlib::EDLIB_STATUS_OK && result.editDistance >= 0) {
        if (result.numLocations > 0) {
            score = result.editDistance;
            end_pos = result.endLocations[0];
        }
    }
    edlib::edlibFreeAlignResult(result);
    return score;
}

