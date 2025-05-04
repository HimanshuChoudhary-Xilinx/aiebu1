// SPDX-License-Identifier: MIT
// Copyright (C) 2025, Advanced Micro Devices, Inc. All rights reserved.

#include <iostream>
#include "aie2ps_report.h"


namespace aiebu {

void
aie2ps_report::
addpage(page& lpage, assembler_state &page_state, offset_type textsize, offset_type datasize)
{
  std::vector<jobid_type> jobs = page_state.m_jobids;
  jobs.erase(std::remove(jobs.begin(), jobs.end(), "EOF"), jobs.end());
  m_colpages[lpage.get_colnum()].emplace_back(lpage.get_colnum(), lpage.get_pagenum(), textsize, datasize, jobs, page_state.m_localbarriermap, page_state.m_joblaunchmap);
}

std::vector<char>
aie2ps_report::
generate()
{
   std::cout << "************************** ASSEMBLER REPORT **************************" << std::endl;
   std::cout << "BUILD ID: " << m_build_id << std::endl;
   for (auto& col : m_colpages)
   {
     std::cout << "COLUMN: "<< col.first << std::endl;
     for (auto& page : col.second)
     {
       std::cout << "\tPAGE NO.:          " << page.m_pagenum << std::endl;
       std::cout << "\tJOBS:              ";
       for (auto j : page.m_jobids)
         std::cout << j << ",";
       std::cout << std::endl;
       std::cout << "\tSECTIONS:          .ctrltext." << page.m_colnum << "." << page.m_pagenum << ", .ctrldata." << page.m_colnum << "." << page.m_pagenum << std::endl;
       std::cout << "\tTEXT SECTION SIZE: " << page.m_textsize << std::endl;
       std::cout << "\tDATA SECTION SIZE: " << page.m_datasize << std::endl;
       std::cout << "\tGRAPH:" << std::endl;
       for (auto& b : page.m_localbarriermap)
       {
         std::cout << "\t      local barrier {" << b.first  << "}: jobids ";
         for (auto& j : b.second) 
           std::cout << j << ",";
         std::cout << std::endl;
       }

       for (auto& b : page.m_joblaunchmap)
       {
         std::cout << "\t      job launchers for {" << b.first  << "}: jobids ";
         for (auto j : b.second) 
           std::cout << j << ",";
         std::cout << std::endl;
       }
       std::cout << std::endl;   
     }

   }
    return {};
}

}
