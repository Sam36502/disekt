#include "../include/recon.h"


REC_TestResult TEST_ValidateChecksum(NYB_DataBlock block) {

	//	Check if we have the metadata
	//	If the err is not a valid disk-error code; just ignore the 
	if ((block.err_code & 0x80) == 0) return TEST_SKIPPED;

	if (block.checksum == DSK_Checksum(block.data)) return TEST_PASSED;
	else return TEST_FAILED;
}

REC_TestResult TEST_CheckBAM(NYB_DataBlock block) {
	return TEST_SKIPPED;
}


REC_Test REC_TEST_LIST[] = {
	{
		.name = "Validate Checksum",
		.test_func = &TEST_ValidateChecksum
	},
	{
		.name = "Check BAM Format",
		.test_func = &TEST_CheckBAM
	}
};

#define REC_TEST_COUNT (sizeof(REC_TEST_LIST) / sizeof(REC_Test))
