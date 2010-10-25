/** table.cc --- 
 *
 * Copyright (C) 2010 Nil Geisweiller
 *
 * Author: Nil Geisweiller <ngeiswei@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include "table.h"

#include <opencog/util/numeric.h>

#include "ann.h"
#include "simple_nn.h"
#include "convert_ann_combo.h"

namespace combo {

using opencog::sq;

truth_table::size_type
truth_table::hamming_distance(const truth_table& other) const
{
    OC_ASSERT(other.size() == size(),
              "truth_tables size should be the same.");

    size_type res = 0;
    for (const_iterator x = begin(), y = other.begin();x != end();)
        res += (*x++ != *y++);
    return res;
}

bool truth_table::same_truth_table(const combo_tree& tr) const {
    const_iterator cit = begin();
    for (int i = 0; cit != end(); ++i, ++cit) {
        for (int j = 0; j < _arity; ++j)
            binding(j + 1) = bool_to_vertex((i >> j) % 2);
        if(*cit != vertex_to_bool(eval(*_rng, tr)))
            return false;
    }
    return true;
}

partial_truth_table::partial_truth_table(const combo_tree& tr,
                                         const truth_table_inputs& tti,
                                         opencog::RandGen& rng) {
    for(bm_cit i = tti.begin(); i != tti.end(); ++i) {
        tti.set_binding(*i);
        //assumption : all inputs of t are contin_t
        vertex res = eval_throws(rng, tr);
        OC_ASSERT(is_boolean(res), "res must be boolean");
        push_back(vertex_to_bool(res));
    }
}

contin_input_table::contin_input_table(int sample_count, int arity,
                                       opencog::RandGen& rng, 
                                       double max_randvalue,
                                       double min_randvalue)
{
    //populate the matrix
    for (int i = 0; i < sample_count; ++i) {
        contin_vector cv;
        for (int j = 0; j < arity; ++j)
        cv.push_back((max_randvalue - min_randvalue) 
                     * rng.randdouble() + min_randvalue); 
        // input interval
        push_back(cv);
    }
}

contin_table::contin_table(const combo_tree& tr, const contin_input_table& cti,
                           opencog::RandGen& rng)
{
    OC_ASSERT(!tr.empty());
    if(is_ann_type(*tr.begin())) { 
        // we treat ANN differently because they must be decoded
        // before being evaluated. Also note that if there are memory
        // neurones then the state of the network is evolving at each
        // input, so the order with contin_input_table does matter
        ann net = tree_transform().decodify_tree(tr);
        int depth = net.feedforward_depth();
        for(const_cm_it i = cti.begin(); i != cti.end(); ++i) {
            contin_vector tmp(*i);
            tmp.push_back(1.0); // net uses that in case the function
                                // to learn needs some kind of offset
            net.load_inputs(tmp);
            dorepeat(depth)
                net.propagate();
            push_back(net.outputs[0]->activation);
        }
    } else {
        for(const_cm_it i = cti.begin(); i != cti.end(); ++i) {
            cti.set_binding(*i);
            // assumption : all inputs and output of tr are contin_t
            // this assumption can be verified using infer_type_tree
            vertex res = eval_throws(rng, tr);
            push_back(get_contin(res));
        }
    }
}

bool contin_table::operator==(const contin_table& ct) const
{
    if (ct.size() == size()) {
        const_cv_it ct_i = ct.begin();
        for (const_cv_it i = begin(); i != end(); ++i, ++ct_i) {
            if (!isApproxEq(*i, *ct_i))
                return false;
        }
        return true;
    } else return false;
}

contin_t contin_table::abs_distance(const contin_table& other) const
{
    OC_ASSERT(other.size() == size(),
              "contin_tables should have the same size.");

    contin_t res = 0;
    for (const_iterator x = begin(), y = other.begin();x != end();)
        res += fabs(*x++ -*y++);
    return res;
}

contin_t contin_table::sum_squared_error(const contin_table& other) const
{
    OC_ASSERT(other.size() == size(),
              "contin_tables should have the same size.");

    contin_t res = 0;
    for (const_iterator x = begin(), y = other.begin();x != end();)
        res += sq(*x++ -*y++);
    return res;
}

contin_t contin_table::mean_squared_error(const contin_table& other) const
{
    OC_ASSERT(other.size() == size() && size() > 0,
              "contin_tables should have the same size > 0.");
    return sum_squared_error(other) / (contin_t)other.size();
}

contin_t contin_table::root_mean_square_error(const contin_table& other) const
{
    OC_ASSERT(other.size() == size() && size() > 0,
              "contin_tables should have the same size > 0.");
    return sqrt(mean_squared_error(other));
}

bool checkCarriageReturn(std::istream& in) {
    char next_c = in.get();
    if(next_c == '\r') // DOS format
        next_c = in.get();
    if(next_c == '\n')
        return true;
    return false;
}

arity_t istreamArity(std::istream& in) {
    arity_t arity = -1;
    while(!in.eof()) {
        arity++;
        std::string str;
        in >> str;
        if(checkCarriageReturn(in))
            return arity;
    }
    return arity;
}

} // ~namespace combo
