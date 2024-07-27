#include "pin.H"
#include <iostream>
#include <map>
#include <string>
#include <fstream>

struct rtn_stat{
  std:: string rtn_name;
  std::string img_name;
  ADDRINT img_address;
  ADDRINT rtn_address;
  int inst_count;
  int num_calls;
};


std::map<ADDRINT, rtn_stat> rtn_map; 



/* ===================================================================== */

VOID PIN_FAST_ANALYSIS_CALL docount(ADDRINT rtn_addr){
  rtn_map[rtn_addr].inst_count++; //counter of the instructions of the specific rtn
}

VOID PIN_FAST_ANALYSIS_CALL count_calls(ADDRINT rtn_addr){
  rtn_map[rtn_addr].num_calls++;
}



bool sortByInstCount(const std::pair<ADDRINT, rtn_stat>& a, const std::pair<ADDRINT, rtn_stat>& b) {
    return a.second.inst_count > b.second.inst_count; 
}

/* ===================================================================== */

VOID Instruction(INS ins, VOID *v)
{
  RTN rtn = INS_Rtn(ins); // deriving the RTN from the instruction
  if (!RTN_Valid(rtn)){
    return;
  }
  RTN_Open(rtn);
  const std::string& rtn_name = RTN_Name(rtn); // deriving the rtn name using the rtn object
  ADDRINT rtn_addr = RTN_Address(rtn);
  IMG img_from_rtn = IMG_FindByAddress(rtn_addr);
  const std::string& img_name = IMG_Name(img_from_rtn);
  ADDRINT img_addr = IMG_LowAddress(img_from_rtn);
  //Check if the rtn is already in the map
  auto it = rtn_map.find(rtn_addr);
  if (it == rtn_map.end()) { //Does not exist in the map, need to create new entry
    rtn_stat tmp_struct;
    tmp_struct.rtn_name = rtn_name;
    tmp_struct.img_name = img_name;
    tmp_struct.img_address = img_addr;
    tmp_struct.rtn_address = rtn_addr;
    tmp_struct.inst_count = 0;
    tmp_struct.num_calls = 0;
    rtn_map[rtn_addr] = tmp_struct;
  }
  RTN_Close(rtn);
  INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount,IARG_FAST_ANALYSIS_CALL, IARG_PTR, rtn_addr, IARG_END); // call improved docount

  
}


VOID RoutineInstrumentation(RTN rtn, void *v){
  RTN_Open(rtn);
  ADDRINT rtn_addr = RTN_Address(rtn);
  RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)count_calls, IARG_FAST_ANALYSIS_CALL, IARG_PTR, rtn_addr, IARG_END); //call rtn counter function
  RTN_Close(rtn);
}


/* ===================================================================== */

VOID Fini(INT32 code, VOID *v)
{
  std::ofstream out_file;
  out_file.open("rtn-output.csv");
  std::vector<std::pair<ADDRINT, rtn_stat>> sorted_pairs(rtn_map.begin(), rtn_map.end());
  // Sort the vector of pairs by struct field val3 using the custom comparison function
  std::sort(sorted_pairs.begin(), sorted_pairs.end(), sortByInstCount);
  for (auto& pair : sorted_pairs) {
    rtn_stat& struct_member = pair.second;
    out_file << struct_member.img_name << ", " << "0x" << struct_member.img_address << ", " << struct_member.rtn_name << ", " << "0x" << struct_member.rtn_address << ", " << struct_member.inst_count << ", " << struct_member.num_calls << std::endl;
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
  
  
  INS_AddInstrumentFunction(Instruction, 0);
  RTN_AddInstrumentFunction(RoutineInstrumentation, 0);
  PIN_AddFiniFunction(Fini, 0);
  
  PIN_StartProgram();
  
  return 0;
}




