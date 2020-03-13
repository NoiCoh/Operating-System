#include "os.h"

void page_table_update(uint64_t pt, uint64_t vpn, uint64_t ppn){

	vpn = vpn >> 12;
	
	uint64_t* PT_1 = phys_to_virt(pt);
	uint64_t PTE_1 = vpn & 0x1ff;
	uint64_t pt_2 = alloc_page_frame() << 12;
	PT_1[PTE_1] = (pt_2 | 1);
	
	vpn = vpn >> 9;
	uint64_t* PT_2 = phys_to_virt(pt_2);
	uint64_t PTE_2 = vpn & 0x1ff;
	uint64_t pt_3 = alloc_page_frame() << 12;
	PT_2[PTE_2] = (pt_3 | 1);
	    
	vpn = vpn >> 9;
	uint64_t* PT_3 = phys_to_virt(pt_3);
	uint64_t PTE_3 = vpn & 0x1ff;
	uint64_t pt_4 = alloc_page_frame() << 12;
	PT_3[PTE_3] = (pt_4  | 1);
	
	vpn = vpn >> 9;
	uint64_t* PT_4 = phys_to_virt(pt_4);
	uint64_t PTE_4 = vpn & 0x1ff;
	uint64_t pt_5 = alloc_page_frame() << 12;
	PT_4[PTE_4] = (pt_5  | 1);
	    
	vpn = vpn >> 9;
	uint64_t* PT_5 = phys_to_virt(pt_5);
	uint64_t PTE_5 = vpn & 0x1ff;
	PT_5[PTE_5] = ppn;

}

uint64_t page_table_query(uint64_t pt, uint64_t vpn){
	
	vpn = vpn >> 12;
	    
	uint64_t* PT_1 = phys_to_virt(pt);
	uint64_t PTE_1 = vpn & 0x1ff;
	uint64_t pt_2 = PT_1[PTE_1];
	if(!(pt_2 & 0x1)) return NO_MAPPING;
	pt_2 = ((pt_2 >> 1) << 1);
	
	vpn = vpn >> 9;
	uint64_t* PT_2 = phys_to_virt(pt_2);
	uint64_t PTE_2 = vpn & 0x1ff;
	uint64_t pt_3 = PT_2[PTE_2];
	if(!(pt_3 & 0x1)) return NO_MAPPING;
	pt_3 = ((pt_3 >> 1) << 1);
	
	vpn = vpn >> 9;
	uint64_t* PT_3 = phys_to_virt(pt_3);
	uint64_t PTE_3 = vpn & 0x1ff;
	uint64_t pt_4 = PT_3[PTE_3];
	if(!(pt_4 & 0x1)) return NO_MAPPING;
	pt_4 = ((pt_4 >> 1) << 1);
	    
	vpn = vpn >> 9;
	uint64_t* PT_4 = phys_to_virt(pt_4);
	uint64_t PTE_4 = vpn & 0x1ff;
	uint64_t pt_5 = PT_4[PTE_4];
	if(!(pt_5 & 0x1)) return NO_MAPPING;
	pt_5 = ((pt_5 >> 1) << 1);
	
	vpn = vpn >> 9;
	uint64_t* PT_5 = phys_to_virt(pt_5);
	uint64_t PTE_5 = vpn & 0x1ff;
	uint64_t ppn = PT_5[PTE_5];
	return ppn;
}