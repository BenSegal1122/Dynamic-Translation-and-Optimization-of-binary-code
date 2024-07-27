#include <iostream>
#include <iomanip>
#include <fstream>
#include <map>
#include <vector>
#include "pin.H"
using namespace std;
using std::setw;
using std::cerr;
using std::cout;
using std::string;
using std::endl;

typedef struct LoopStat{
  ADDRINT loop_target_address;
  int count_seen;
  int count_taken_branch;
  int count_invoked;
  string rtn_name;
  ADDRINT loop_rtn_address;
  int count_diff;
  int* inst_count_of_loop_rtn;
  int* loop_rtn_calls;
  int prev_iterations_count;
  int curr_iterations_count;
  struct LoopStat * next;
} LOOP_STAT;

typedef struct RtnStat{
  string rtn_name;
  ADDRINT rtn_address;
  int inst_count;
  int num_calls;
  struct RtnStat * next;
  struct LoopStat * routine_loops;
} RTN_STAT;

RTN_STAT* routines_list = nullptr;
  

/*
 * count_seen = total number of times the loop's backward edge was executed
 * count_invoked = number of times the loop was invoked
 * mean_taken = average number of iterations taken for the loop invocations
 * diff_count = number of times that two successice loop invocations took a different number of iterations
 * rtn_name = routine name in which the loop reside
 * loop_rtn_calls = number of times the routine was called
 * rtn_address = routine address in which the loop resides
 * inst_count_of_loop_rtn = instructions count of the routine containing the loop
*/

inline VOID PIN_FAST_ANALYSIS_CALL docount(UINT64 * inst_counter){
  (*inst_counter)++; //counter of the instructions of the specific rtn
}

inline VOID PIN_FAST_ANALYSIS_CALL count_calls(UINT64 * calls_counter){
  (*calls_counter)++;
}



inline bool sortByCountSeen(LOOP_STAT* a, LOOP_STAT* b) {
    return a->count_seen > b->count_seen; 
}


inline VOID docount_inc_branch(INT32 taken, LOOP_STAT* ls){
  ls->count_seen++;
  if (taken) {
    ls->count_taken_branch++;
    ls->curr_iterations_count++;
  }
  else{
    ls->count_invoked++;
    if (ls->prev_iterations_count != -1 && ls->curr_iterations_count != ls->prev_iterations_count) {
      ls->count_diff++;
    }
    ls->prev_iterations_count = ls->curr_iterations_count;
    ls->curr_iterations_count = 0;
  }
}

VOID RoutineInstrumentation(RTN rtn, void *v){
  if (!RTN_Valid(rtn)){
    return;
  }

  string rtn_name = RTN_Name(rtn);
  ADDRINT rtn_addr = RTN_Address(rtn);
  RTN_STAT * rs = new RTN_STAT;
  rs->rtn_name = rtn_name;
  rs->rtn_address = rtn_addr;
  rs->inst_count = 0;
  rs->num_calls = 0;
  rs->routine_loops = nullptr;
  rs->next = routines_list;
  routines_list = rs;
  RTN_Open(rtn);
  RTN_InsertCall(rtn,IPOINT_BEFORE, (AFUNPTR)count_calls, IARG_FAST_ANALYSIS_CALL, IARG_PTR,&(rs->num_calls), IARG_END);
  for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins)){
    //if (INS_IsDirectControlFlow(ins) && !INS_IsCall(ins) && INS_IsBranch(ins)){
    if (INS_IsDirectControlFlow(ins)&& !INS_IsCall(ins) && INS_Category(ins) == XED_CATEGORY_COND_BR){
      ADDRINT current_address = INS_Address(ins);
      ADDRINT target_address = INS_DirectControlFlowTargetAddress(ins);
      
      if (current_address > target_address){ // jump backwards -> loop
	LOOP_STAT * ls = new LOOP_STAT;
	ls->loop_target_address = target_address;
	ls->count_seen = 0;
	ls->count_taken_branch = 0;
	ls->count_invoked = 0;
	ls->rtn_name = rtn_name;
	ls->loop_rtn_address = rtn_addr;
	ls->count_diff = 0;
	ls->inst_count_of_loop_rtn = &(rs->inst_count);
	ls->loop_rtn_calls = &(rs->num_calls);
	ls->prev_iterations_count = -1;
	ls->curr_iterations_count = 0;
	ls->next = rs->routine_loops;
	rs->routine_loops = ls;
	INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount_inc_branch, IARG_FAST_ANALYSIS_CALL, IARG_BRANCH_TAKEN, IARG_PTR, ls,  IARG_END);
      }
      INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount, IARG_FAST_ANALYSIS_CALL, IARG_PTR, &(rs->inst_count), IARG_END);
    }
  }
  
  RTN_Close(rtn);  
}

VOID Fini(INT32 code, VOID *v)
{
  ofstream out_file;
  vector<LOOP_STAT*> loops_vector;
  out_file.open("loop-count.csv");
  for (RTN_STAT* rtn = routines_list; rtn; rtn = rtn->next){
    for (LOOP_STAT* loop = rtn->routine_loops; loop; loop = loop->next){
      if (loop->count_seen > 0){
	loops_vector.emplace_back(loop);
      } 
    }
  }
  
  std::sort(loops_vector.begin(), loops_vector.end(), sortByCountSeen);

  for (auto& loop : loops_vector) {
    out_file << "0x" << hex << loop->loop_target_address << ", "
	     << dec << loop->count_seen << ", "
	     << loop->count_invoked << ", "
	     << float(loop->count_taken_branch) / float(loop->count_invoked) << ", "
	     << loop->count_diff << ", "
	     << loop->rtn_name << ", "
	     << "0x" << hex << loop->loop_rtn_address << ", "
	     << dec << *(loop->inst_count_of_loop_rtn) << ", "
	     << *(loop->loop_rtn_calls) << endl;
  }
      
  out_file.close();
}

INT32 Usage()
{
  std::cerr <<
    "This tool prints out the number of dynamic instructions executed to stderr.\n"
    "\n";
  
  std::cerr << KNOB_BASE::StringKnobSummary();

  std::cerr << std::endl;

    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
  PIN_InitSymbols();
  
  if( PIN_Init(argc,argv) )
    {
      return Usage();
    }
  
  RTN_AddInstrumentFunction(RoutineInstrumentation, 0);
  PIN_AddFiniFunction(Fini, 0);
  
  PIN_StartProgram();
  
  return 0;
}




