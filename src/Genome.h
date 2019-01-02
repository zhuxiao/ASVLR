#ifndef SRC_GENOME_H_
#define SRC_GENOME_H_

#include <iostream>
#include <string>
#include <htslib/sam.h>
#include <htslib/faidx.h>

#include "Paras.h"
#include "Chrome.h"
#include "util.h"

using namespace std;

#define MIN_VALID_TRA_RATIO			(0.9f)

class Genome{
	private:
		Paras *paras;
		vector<Chrome*> chromeVector;
		faidx_t *fai;
		bam_hdr_t *header;
		int genomeSize;

		// output files
		string out_dir_detect = "1_candidates";    // "1_candidates"
		string out_dir_assemble = "2_assemble";  // "2_assemble"
		string out_dir_call = "3_call";      // "3_call"

		string out_filename_detect_snv, out_filename_detect_indel, out_filename_detect_clipReg;
		string out_filename_call_snv, out_filename_call_indel, out_filename_call_clipReg, out_filename_call_tra;

		//vector<varCand*> var_cand_vec;

	public:
		Genome(Paras *paras);
		virtual ~Genome();
		int generateGenomeBlocks();
		void saveGenomeBlocksToFile();
		int genomeDetect();
		int genomeLocalAssemble();
		int genomeCall();
		void estimateSVSizeNum();
		void saveResultVCFDetect();
		void saveResultVCF();

	private:
		void init();
		Chrome* allocateChrome(string& chrname, int chrlen, faidx_t *fai);
		int getGenomeSize();
		void destroyChromeVector();
		int computeCoverage();
		void removeRedundantTra();
		mateClipReg_t* genomeGetOverlappedMateClipReg(mateClipReg_t *clip_reg_given, vector<Chrome*> &chrome_vec);
		void saveDetectResultToFile();
		void mergeDetectResult();
		void loadCallData();
		void genomeCallTra();
		varCand* constructNewVarCand(varCand *var_cand, varCand *var_cand_tmp);
		vector<int32_t> computeTraLoc(varCand *var_cand, varCand *var_cand_tmp);
		vector<int32_t> getMateTraReg(size_t query_id, size_t start_query_pos, size_t end_query_pos, varCand *var_cand_tmp);
		bool isValidTraReg(size_t start_query_pos, size_t end_query_pos, size_t start_mate_query_pos, size_t end_mate_query_pos);
		vector<int32_t> computeTraClippingLoc(size_t query_clip_part_flag, varCand *var_cand, vector<int32_t> &aln_idx_vec, varCand *var_cand_tmp, vector<int32_t> &mate_aln_idx_vec);
		bool computeTraCallSuccessFlag(vector<int32_t> &tra_loc_vec);
		void genomeFillVarseq();
		void genomeSaveCallSV2File();
		void saveTraCall2File();
		//void removeFPs();
		void mergeCallResult();
		void saveIndelVCFDetect(string &in, string &out_vcf);
		void saveSnvVCFDetect(string &in, string &out_vcf);
		void saveIndelVCF(string &in, string &out_vcf);
		void saveSnvVCF(string &in, string &out_vcf);
		void saveVCFHeader(ofstream &fp);
};


#endif /* SRC_GENOME_H_ */