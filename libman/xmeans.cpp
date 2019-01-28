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

#include <iostream>
#include <cassert>

#include "xmeans.hpp"
#include "types.hpp"
#include "util.hpp"
#include "io.hpp"
#include "clusters.hpp"

namespace knor {

//void xmeans::run() {
    //switch(state) {
        //case TEST:
            //test();
            //break;
        //case ALLOC_DATA:
            //numa_alloc_mem();
            //break;
        //case H_EM:
            //H_EM_step();
            //break;
        //case H_SPLIT:
            //H_split_step();
            //break;
        //case MEAN:
            //partition_mean();
            //break;
        //case EXIT:
            //throw base::thread_exception(
                    //"Thread state is EXIT but running!\n");
        //default:
            //throw base::thread_exception("Unknown thread state\n");
    //}
    //sleep();
//}

void xmeans::start(const thread_state_t state=WAIT) {
    this->state = state;
    int rc = pthread_create(&hw_thd, NULL, callback<xmeans>, this);
    if (rc)
        throw base::thread_exception(
                "Thread creation (pthread_create) failed!", rc);
}

void xmeans::H_EM_step() {
    local_hcltrs.clear();

    nchanged.assign(base::get_max_hnodes(k*2), 0);

    auto itr = g_hcltrs->get_iterator();
    while (itr.has_next()) {
        auto kv = itr.next();
        if (kv.second->has_converged()) {
            continue;
        }
            // No need to set id, zeroid, oneid because global hcltrs knows them
            local_hcltrs[kv.first] = base::h_clusters::create(2, ncol);
            local_hcltrs[kv.first]->clear(); // NOTE: Could be combined into ctor
    }

    for (unsigned row = 0; row < nprocrows; row++) {
        // What cluster is this row in?
        unsigned true_row_id = get_global_data_id(row);
        auto rpart_id = part_id[true_row_id];

        // Not active
        if (!(cltr_active_vec[cluster_assignments[true_row_id]]) ||
                g_hcltrs->at(rpart_id)->has_converged())
            continue; // Skip it

        unsigned asgnd_clust = base::INVALID_CLUSTER_ID;
        bool flag = 0; // Is the best the zeroid or oneid?
        double best, dist;
        dist = best = std::numeric_limits<double>::max();

        for (unsigned clust_idx = 0; clust_idx < 2; clust_idx++) {
            dist = base::dist_comp_raw<double>(&local_data[row*ncol],
                    &(g_hcltrs->at(rpart_id)->
                        get_means()[clust_idx*ncol]), ncol, dist_metric);
            if (dist < best) {
                best = dist;
                if (clust_idx == 0) {
                    asgnd_clust = g_hcltrs->at(rpart_id)->get_zeroid();
                } else {
                    asgnd_clust = g_hcltrs->at(rpart_id)->get_oneid();
                    flag = 1;
                }
            }
        }

        assert(asgnd_clust != base::INVALID_CLUSTER_ID);

        if (asgnd_clust != cluster_assignments[true_row_id])
            nchanged[rpart_id]++;
        local_hcltrs[rpart_id]->add_member(&local_data[row*ncol], flag);
        cluster_assignments[true_row_id] = asgnd_clust;
    }
}
} // End namespace knor
