/* Copyright 2020 HPS/SAFARI Research Groups
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "perceptron.h"

#include <vector>

extern "C" {
#include "bp/bp.param.h"
#include "core.param.h"
#include "globals/assert.h"
#include "statistics.h"
}

#define DEBUG(proc_id, args...) _DEBUG(proc_id, DEBUG_BP_DIR, ##args)

namespace {

struct Perceptron_State {
  std::vector<Perceptron> perceptronTable;
};

std::vector<Perceptron_State> perceptron_state_all_cores;

uns32 get_perceptron_table_index(const Addr addr) {
  return addr % NUM_PERCEPTRON;
}
}  // namespace

void bp_perceptron_timestamp(Op* op) {}
void bp_perceptron_recover(Recovery_Info* info) {}
void bp_perceptron_spec_update(Op* op) {}
void bp_perceptron_retire(Op* op) {}


void bp_perceptron_init() {
  uns ii;
  perceptron_state_all_cores.resize(NUM_CORES);
  for(auto& perceptron_state : perceptron_state_all_cores) {
    for(ii=0; ii<NUM_PERCEPTRON; ii++) {
      Perceptron* perceptron = new Perceptron();
      perceptron->weights = (int32*)malloc(sizeof(int32) * (HIST_LENGTH + 1));
      perceptron_state.perceptronTable.push_back(*perceptron);
    }
  }
}

uns8 bp_perceptron_pred(Op* op) {
  const uns   proc_id      = op->proc_id;
  const auto& perceptron_state = perceptron_state_all_cores.at(proc_id);

  const Addr  addr      = op->oracle_info.pred_addr;
  const uns32 hist      = op->oracle_info.pred_global_hist;
  const uns32 percpetron_index = get_perceptron_table_index(addr);
  const Perceptron  perceptron = perceptron_state.perceptronTable[percpetron_index];

  uns8  pred;
  int32       output   = 0;
  uns         ii;
  uns64       mask;

  int32* w = perceptron.weights;
  output = *(w++); // Bias weight.
  for(mask = ((uns64)1) << 63, ii = 0; ii < HIST_LENGTH; ii++, mask >>= 1, w++) {
    if(!!(hist & mask))
      output += *w;
    else
      output += -(*w);
  }

  pred = (output < PERCEPTRON_THRESHOLD) ? 1 : 0;

  DEBUG(proc_id, "Predicting with perceptron for  op_num:%s  index:%d\n",
        unsstr64(op->op_num), percpetron_index);
  DEBUG(proc_id, "Predicting  addr:%s  percpetron_index:%u  pred:%d  dir:%d\n",
        hexstr64s(addr), percpetron_index, pred, op->oracle_info.dir);

  return pred;
}

void bp_perceptron_update(Op* op) {
  if(op->table_info->cf_type != CF_CBR) {
    // If op is not a conditional branch, we do not interact with perceptron.
    return;
  }

  const uns   proc_id      = op->proc_id;
  auto&       perceptron_state = perceptron_state_all_cores.at(proc_id);
  const Addr  addr         = op->oracle_info.pred_addr;
  const uns32 hist         = op->oracle_info.pred_global_hist;
  const uns32 percpetron_index    = get_perceptron_table_index(addr);
  const Perceptron  perceptron    = perceptron_state.perceptronTable[percpetron_index];

  DEBUG(proc_id, "Updating perceptron branch predictor for op_num:%s  perceptron_index:%d  dir:%d\n",
        unsstr64(op->op_num), percpetron_index, op->oracle_info.dir);

  // TODO(Charles): Perceptron update.
  if(op->oracle_info.dir) {
    // Branch taken.
  } else {
    // Branch not taken.
  }

  DEBUG(proc_id, "Updating addr:%s  perceptron:%u dir:%d\n", hexstr64s(addr),
        percpetron_index, op->oracle_info.dir);
}
