//***************************************************************************
//* Copyright (c) 2020 Saint Petersburg State University
//* All Rights Reserved
//* See file LICENSE for details.
//***************************************************************************

#pragma once

#include <cstdlib>

namespace kmer {

template<class Seq>
class KMerBucketPolicy {
    typedef typename Seq::hash hash;

public:
    explicit KMerBucketPolicy(size_t num_buckets = 0)
            : num_buckets_(num_buckets) {}

    void reset(size_t num_buckets) {
        num_buckets_ = num_buckets;
    }

    size_t num_buckets() const { return num_buckets_; }

    size_t operator()(const Seq &s) const {
        return hash()(s) % num_buckets_;
    }

    template<class Ref>
    size_t operator()(Ref s) const {
        return hash()(s) % num_buckets_;
    }

private:
    size_t num_buckets_ = 0;
};

}
