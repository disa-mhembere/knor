/*
 * Copyright 2016 neurodata (http://neurodata.io/)
 * Written by Disa Mhembere (disa@jhu.edu)
 *
 * This file is part of knor
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY CURRENT_KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __KNOR_BASE_HPP__
#define __KNOR_BASE_HPP__

#include <pthread.h>

#include <vector>
#include <unordered_map>
#include <memory>
#include <atomic>

#include "types.hpp"
#include "thread_state.hpp"
#include "exception.hpp"

#ifdef PROFILER
#include <gperftools/profiler.h>
#endif

namespace knor {

class thread;

class base {
protected:
    std::string fn; // file on disk
    const size_t nrow, ncol;
    unsigned k, max_iters;
    unsigned nnodes, nthreads;
    core::init_t _init_t;
    double tolerance;
    core::dist_t _dist_t;
    size_t num_changed; // total # samples changed in an iter
    // how many threads have not completed their task
    std::atomic<unsigned> pending_threads;

    std::vector<unsigned> cluster_assignments;
    std::vector<llong_t>cluster_assignment_counts;
    std::vector<unsigned> thd_max_row_idx;

    // threading
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    pthread_mutexattr_t mutex_attr;
    std::vector<std::shared_ptr<thread> > threads;

    base(const std::string fn, const size_t nrow,
            const size_t ncol, const unsigned k, const unsigned max_iters,
            const unsigned nnodes, const unsigned nthreads,
            const double* centers, const core::init_t it,
            const double tolerance, const core::dist_t dt);

public:
    const size_t get_num_changed() const { return num_changed; }
    typedef std::shared_ptr<base> ptr;
    typedef std::vector<std::shared_ptr
        <thread> >::iterator thread_iter;

    // pass file handle to threads to read & numa alloc
    virtual void random_partition_init() {
        throw core::parameter_exception("Unsupported initialization type");
    };
    virtual void kmeanspp_init() {
        throw core::parameter_exception("Unsupported initialization type");
    };
    virtual void forgy_init() {
        throw core::parameter_exception("Unsupported initialization type");
    };

    virtual core::cluster_t run(
            double* allocd_data=NULL, const bool numa_opt=false) = 0;

    std::vector<std::shared_ptr<thread> >& get_threads() {
        return threads;
    }

    virtual void run_init();
    void set_thd_dist_v_ptr(double* v);
    void wake4run(thread_state_t state);
    const double* get_thd_data(const unsigned row_id) const;
    std::pair<unsigned, unsigned> get_rid_len_tup(const unsigned thd_id);
    void set_thread_clust_idx(const unsigned clust_idx);
    virtual const void print_thread_data();
    virtual void destroy_threads();
    virtual void set_thread_data_ptr(double* allocd_data);
    void const print_thread_start_rids();
    double reduction_on_cuml_sum();
    void wait4complete();

    virtual void set_global_ptrs() { throw core::abstract_exception(); };
    virtual void build_thread_state() { throw core::abstract_exception(); };
    const unsigned* get_cluster_assignments() const {
        return &cluster_assignments[0];
    }

    void clear_cluster_assignments() {
        std::fill(&cluster_assignments[0],
                &cluster_assignments[nrow], core::INVALID_CLUSTER_ID);
    }
    const size_t get_nrow() { return nrow; }
    const size_t get_ncol() { return ncol; }
    virtual ~base();
};
} // namespace knor
#endif