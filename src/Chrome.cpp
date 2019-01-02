#include <sys/stat.h>
#include <pthread.h>
#include "Chrome.h"
#include "Thread.h"
#include "varCand.h"
#include "util.h"

// Constructor with parameters
Chrome::Chrome(string& chrname, int chrlen, faidx_t *fai, Paras *paras){
	this->paras = paras;
	this->chrname = chrname;
	this->chrlen = chrlen;
	this->fai = fai;
	init();

	blocks_out_file = chrname + "_blocks.bed";
	out_dir_detect = "";
	out_dir_assemble = "";
	out_dir_call = "";

	out_filename_detect_snv = "";
	out_filename_detect_indel = "";
	out_filename_detect_clipReg = "";
	out_filename_call_snv = "";
	out_filename_call_indel = "";
	out_filename_call_clipReg = "";
}

//Destructor
Chrome::~Chrome(){
	if(!blockVector.empty()) destroyBlockVector();
	if(!var_cand_vec.empty()) destroyVarCandVector(var_cand_vec);
	if(!mis_aln_vec.empty()) destroyMisAlnVector();
	if(!clipRegVector.empty()) destroyClipRegVector();
	if(!mateClipRegVector.empty()) destroyMateClipRegVector();
	if(!var_cand_clipReg_vec.empty()) destroyVarCandVector(var_cand_clipReg_vec);
}

// initialization
void Chrome::init(){
	blockNum = 0;
}

// set the output directory
void Chrome::setOutputDir(string& out_dir_detect_prefix, string& out_dir_assemble_prefix, string& out_dir_call_prefix){
	out_dir_detect = out_dir_detect_prefix + "/" + chrname;
	out_dir_assemble = out_dir_assemble_prefix + "/" + chrname;
	out_dir_call = out_dir_call_prefix + "/" + chrname;

	out_filename_detect_snv = out_dir_detect + "_SNV_candidate";
	out_filename_detect_indel = out_dir_detect + "_INDEL_candidate";
	out_filename_detect_clipReg = out_dir_detect + "_clipReg_candidate";
	out_filename_call_snv = out_dir_call + "_SNV";
	out_filename_call_indel = out_dir_call + "_INDEL";
	misAln_reg_filename = out_dir_detect + "_misaln_reg";
	out_filename_call_clipReg = out_dir_call + "_clipReg";

	var_cand_indel_filename = out_dir_assemble + "_var_cand_indel";
	var_cand_clipReg_filename = out_dir_assemble + "_var_cand_clipReg";
}

// generate the chromosome blocks
int Chrome::generateChrBlocks(){
	int pos, begPos, endPos;
	Block *block_tmp;
	bool headIgnFlag, tailIgnFlag;

	blockNum = 0;
	pos = 1;
	while(pos<=chrlen){
		begPos = pos;
		endPos = pos + paras->blockSize - 1;
		if(chrlen-endPos<=paras->blockSize*0.5){
			endPos = chrlen;
			pos = endPos + 1;
		}else
			pos += paras->blockSize - 2 * paras->slideSize;
		blockNum ++;

		if(begPos==1) headIgnFlag = false;
		else headIgnFlag = true;
		if(endPos==chrlen) tailIgnFlag = false;
		else tailIgnFlag = true;

		block_tmp = allocateBlock(chrname, chrlen, begPos, endPos, fai, headIgnFlag, tailIgnFlag);
		block_tmp->setOutputDir(out_dir_detect, out_dir_assemble, out_dir_call);
		blockVector.push_back(block_tmp);
	}
	blockVector.shrink_to_fit();

	return 0;
}

// allocate the memory for one block
Block *Chrome::allocateBlock(string& chrname, size_t chrlen, int startPos, int endPos, faidx_t *fai, bool headIgnFlag, bool tailIgnFlag){
	Block *block_tmp = new Block(chrname, chrlen, startPos, endPos, fai, paras);
	if(!block_tmp){
		cerr << "Chrome: cannot allocate memory" << endl;
		exit(1);
	}
	block_tmp->setRegIngFlag(headIgnFlag, tailIgnFlag);
	return block_tmp;
}

// free the memory of blockVector
void Chrome::destroyBlockVector(){
	vector<Block*>::iterator bloc;
	for(bloc=blockVector.begin(); bloc!=blockVector.end(); bloc++)
		delete (*bloc);   // free each block
	vector<Block*>().swap(blockVector);
}

// free the memory of var_cand_vec
void Chrome::destroyVarCandVector(vector<varCand*> &var_cand_vec){
	varCand *var_cand;
	size_t i, j, k;
	for(i=0; i<var_cand_vec.size(); i++){
		var_cand = var_cand_vec.at(i);
		for(j=0; j<var_cand->varVec.size(); j++)  // free varVec
			delete var_cand->varVec.at(j);
		for(j=0; j<var_cand->newVarVec.size(); j++)  // free newVarVec
			delete var_cand->newVarVec.at(j);

		blat_aln_t *item_blat;
		for(j=0; j<var_cand->blat_aln_vec.size(); j++){ // free blat_aln_vec
			item_blat = var_cand->blat_aln_vec.at(j);
			for(k=0; k<item_blat->aln_segs.size(); k++) // free aln_segs
				delete item_blat->aln_segs.at(k);
			delete item_blat;
		}
		delete var_cand;   // free each item
	}
	vector<varCand*>().swap(var_cand_vec);
}

// free the memory of mis_aln_vec
void Chrome::destroyMisAlnVector(){
	vector<reg_t*>::iterator it;
	for(it=mis_aln_vec.begin(); it!=mis_aln_vec.end(); it++)  // free mis_aln_vec
		delete *it;
	vector<reg_t*>().swap(mis_aln_vec);
}

// free the memory of clipRegVector
void Chrome::destroyClipRegVector(){
	vector<reg_t*>::iterator it;
	for(it=clipRegVector.begin(); it!=clipRegVector.end(); it++)
		delete *it;
	vector<reg_t*>().swap(clipRegVector);
}

// free the memory of mateClipRegVector
void Chrome::destroyMateClipRegVector(){
	vector<mateClipReg_t*>::iterator it;
	for(it=mateClipRegVector.begin(); it!=mateClipRegVector.end(); it++){
		delete (*it)->leftClipReg;
		delete (*it)->rightClipReg;
		delete *it;
	}
	vector<mateClipReg_t*>().swap(mateClipRegVector);
}

// save the blocks to file
void Chrome::saveChrBlocksToFile(){
	ofstream outfile(blocks_out_file);
	if(!outfile.is_open()){
		cerr << "Chrome: Cannot open file " << blocks_out_file << endl;
		exit(1);
	}

	Block* bloc;
	for(size_t i=0; i<blockVector.size(); i++){
		bloc = blockVector.at(i);
		outfile << bloc->chrname << "\t" << bloc->startPos << "\t" << bloc->endPos << endl;
	}
	outfile.close();
}

// fill the estimation data
void Chrome::chrFillDataEst(size_t op_est){
	int i, pos, begPos, endPos, seq_len;
	Block *block_tmp;
	string reg;
	char *seq;
	bool flag;

	if(chrlen>=MIN_CHR_SIZE_EST){ // minimal chromosome size 50kb
		pos = chrlen / 2;
		while(pos<chrlen-(MIN_CHR_SIZE_EST-MIN_CHR_SIZE_EST)/2){
			flag = true;

			// construct a block with a 10kb size
			begPos = pos - BLOCK_SIZE_EST/2;
			endPos = begPos + BLOCK_SIZE_EST - 1;

			reg = chrname + ":" + to_string(begPos) + "-" + to_string(endPos);
			seq = fai_fetch(fai, reg.c_str(), &seq_len);
			for(i=0; i<seq_len; i++){
				if(seq[i]=='N' or seq[i]=='n'){
					flag = false;
					break;
				}
			}
			free(seq);

			if(!flag){ // invalid region, slide to next region
				pos = endPos + 1;
			}else break;
		}

		if(flag){ // valid region
			cout << "Est region: " << chrname << ":" << begPos << "-" << endPos << endl;
			block_tmp = allocateBlock(chrname, chrlen, begPos, endPos, fai, false, false);
			// fill the data
			block_tmp->blockFillDataEst(op_est);
			delete block_tmp;
		}
	}
}

// detect indels for chrome
int Chrome::chrDetect(){
	mkdir(out_dir_detect.c_str(), S_IRWXU | S_IROTH);  // create the directory for detect command

	chrSetMisAlnRegFile();

	if(paras->num_threads<=1) chrDetect_st();  // single thread
	else chrDetect_mt();  // multiple threads

	// detect mated clip regions for duplications and inversions
	computeMateClipRegDupInv();

	// remove FP indels and Snvs in clipping regions
	removeFPIndelSnvInClipReg();

	// merge the results to single file
	//chrMergeDetectResultToFile();

	chrResetMisAlnRegFile();

	return 0;
}

// single thread
int Chrome::chrDetect_st(){
	Block* bloc;
	for(size_t i=0; i<blockVector.size(); i++){
//		if(i++==2)
		{
			bloc = blockVector.at(i);
			//cout << "detect files:" << bloc->out_dir_detect << "/" << bloc->snvFilenameDetect << ", " << bloc->indelFilenameDetect << endl;
			bloc->blockDetect();
		}
	}

	return 0;
}

// multiple threads
int Chrome::chrDetect_mt(){
	MultiThread mt[paras->num_threads];
	for(size_t i=0; i<paras->num_threads; i++){
		mt[i].setNumThreads(paras->num_threads);
		mt[i].setBlockVec(&blockVector);
		mt[i].setUserThreadID(i);
		if(!mt[i].startDetect()){
			cerr << __func__ << ", line=" << __LINE__ << ": unable to create thread, error!" << endl;
			exit(1);
		}
	}
	for(size_t i=0; i<paras->num_threads; i++){
		if(!mt[i].join()){
			cerr << __func__ << ", line=" << __LINE__ << ": unable to join, error!" << endl;
			exit(1);
		}
	}

	return 0;
}

// detect mated clip regions for duplications and inversions
void Chrome::computeMateClipRegDupInv(){
	size_t i;
	Block *bloc;
	reg_t *reg;
	vector<bool> clip_processed_flag_vec;

	for(i=0; i<blockVector.size(); i++){
		bloc = blockVector.at(i);
		for(size_t j=0; j<bloc->clipRegVector.size(); j++){
			clipRegVector.push_back(bloc->clipRegVector.at(j));
			clip_processed_flag_vec.push_back(false);
		}
	}
	clipRegVector.shrink_to_fit();

	// compute the mate flag for duplications and inversions
	for(i=0; i<clipRegVector.size(); i++){

		//if(i>=76)
		{
			reg = clipRegVector.at(i);

			if(clip_processed_flag_vec.at(i)==false){
				cout << i << ": " << reg->chrname << ":" << reg->startRefPos << "-" << reg->endRefPos << endl;

				clipReg clip_reg(reg->chrname, reg->startRefPos, reg->endRefPos, chrlen, paras->inBamFile, fai);
				clip_reg.computeMateClipReg();

				precessClipRegs(i, clip_processed_flag_vec, clip_reg.mate_clip_reg);
			}
		}
	}

	// remove FPs by detecting false overlapped clip regions
	removeFPClipRegs();
}

// process clip regions and mate clip regions
void Chrome::precessClipRegs(size_t idx, vector<bool> &clip_processed_flag_vec, mateClipReg_t &mate_clip_reg){
	size_t i;
	reg_t *reg;
	mateClipReg_t *clip_reg_new;

	// add mate clip region
	if(mate_clip_reg.leftClipReg or mate_clip_reg.rightClipReg){
		clip_reg_new = new mateClipReg_t();
		clip_reg_new->leftClipReg = mate_clip_reg.leftClipReg;
		clip_reg_new->leftClipPosNum = mate_clip_reg.leftClipPosNum;
		clip_reg_new->rightClipReg = mate_clip_reg.rightClipReg;
		clip_reg_new->rightClipPosNum = mate_clip_reg.rightClipPosNum;
		clip_reg_new->reg_mated_flag = mate_clip_reg.reg_mated_flag;
		clip_reg_new->leftMeanClipPos = mate_clip_reg.leftMeanClipPos;
		clip_reg_new->rightMeanClipPos = mate_clip_reg.rightMeanClipPos;
		clip_reg_new->leftClipPosTra1 = clip_reg_new->rightClipPosTra1 = clip_reg_new->leftClipPosTra2 = clip_reg_new->rightClipPosTra2 = -1;
		clip_reg_new->sv_type = mate_clip_reg.sv_type;
		clip_reg_new->dup_num = mate_clip_reg.dup_num;
		clip_reg_new->valid_flag = mate_clip_reg.valid_flag;
		mateClipRegVector.push_back(clip_reg_new);
	}

	// detect overlapped clip regions
	if(mate_clip_reg.leftClipReg){ // left region
		for(i=0; i<clipRegVector.size(); i++){
			reg = clipRegVector.at(i);
			if(isOverlappedReg(mate_clip_reg.leftClipReg, reg)) // overlapped
				clip_processed_flag_vec.at(i) = true;
		}
	}
	if(mate_clip_reg.rightClipReg){ // right region
		for(i=0; i<clipRegVector.size(); i++){
			reg = clipRegVector.at(i);
			if(isOverlappedReg(mate_clip_reg.rightClipReg, reg)) // overlapped
				clip_processed_flag_vec.at(i) = true;
		}
	}
}

// remove FP clip regions
void Chrome::removeFPClipRegs(){
	mateClipReg_t *mate_clip_reg, *mate_clip_reg_overlapped;
	reg_t *reg, *reg2;
	size_t i, clipPosNum, clipPosNum_overlapped;
	int32_t dist;
	vector<size_t> overlap_result;

	// remove FP clip regions by detecting too long distance
	for(i=0; i<mateClipRegVector.size(); i++){
		mate_clip_reg = mateClipRegVector.at(i);
		if(mate_clip_reg->reg_mated_flag){
			reg = mate_clip_reg->leftClipReg;
			reg2 = mate_clip_reg->rightClipReg;
			if(reg->chrname.compare(reg2->chrname)==0){  // same chrome
				dist = (int32_t)reg2->startRefPos - (int32_t)reg->startRefPos;
				if(dist<0) dist = -dist;
				if(dist>MAX_CLIP_REG_SIZE) mate_clip_reg->valid_flag = false;
				if(reg->startRefPos>reg2->endRefPos) mate_clip_reg->valid_flag = false;
			} // different chrome
		}else mate_clip_reg->valid_flag = false;
	}

	// remove FP clip regions by detecting false overlapped regions
	for(i=0; i<mateClipRegVector.size(); ){
		mate_clip_reg = mateClipRegVector.at(i);
		if(mate_clip_reg->valid_flag and mate_clip_reg->reg_mated_flag){
			mate_clip_reg_overlapped = getOverlappedMateClipReg(mate_clip_reg, mateClipRegVector);
			if(mate_clip_reg_overlapped){
				clipPosNum = mate_clip_reg->leftClipPosNum + mate_clip_reg->rightClipPosNum;
				clipPosNum_overlapped = mate_clip_reg_overlapped->leftClipPosNum + mate_clip_reg_overlapped->rightClipPosNum;
				if(clipPosNum>=clipPosNum_overlapped)
					mate_clip_reg_overlapped->valid_flag = false;
				else{
					mate_clip_reg->valid_flag = false;
					i++;
				}
			}else
				i++;
		}else i++;
	}

//	for(i=0; i<mateClipRegVector.size(); i++){
//		mate_clip_reg = mateClipRegVector.at(i);
//
//		for(j=0; j<2; j++){
//			if(j==0){ // left region
//				reg = mate_clip_reg->leftClipReg;
//				clipPosNum = mate_clip_reg->leftClipPosNum;
//			}else{  // right region
//				reg = mate_clip_reg->rightClipReg;
//				clipPosNum = mate_clip_reg->rightClipPosNum;
//			}
//
//			if(reg){
//				overlap_result = getOverlapClipReg(reg);
//				if(overlap_result.size()>0){
//					idx = overlap_result.at(0);
//					end_flag = overlap_result.at(1);
//					if(end_flag==LEFT_END) clipPosNum_overlap = mateClipRegVector.at(idx)->leftClipPosNum;
//					else clipPosNum_overlap = mateClipRegVector.at(idx)->rightClipPosNum;
//
//					if(clipPosNum>=clipPosNum_overlap) mateClipRegVector.at(idx)->valid_flag = false;
//					else mateClipRegVector.at(i)->valid_flag = false;
//				}
//			}
//		}
//	}

	// remove invalid items
	for(i=0; i<mateClipRegVector.size(); ){
		mate_clip_reg = mateClipRegVector.at(i);
		if(mate_clip_reg->valid_flag==false){
			if(mate_clip_reg->leftClipReg) delete mate_clip_reg->leftClipReg;
			if(mate_clip_reg->rightClipReg) delete mate_clip_reg->rightClipReg;
			delete mate_clip_reg;
			mateClipRegVector.erase(mateClipRegVector.begin()+i);
		}else i++;
	}
}

// get the overlapped clip region
vector<size_t> Chrome::getOverlapClipReg(reg_t *given_reg){
	vector<size_t> overlap_result;
	reg_t *reg;
	for(size_t i=0; i<mateClipRegVector.size(); i++){
		reg = mateClipRegVector.at(i)->leftClipReg;
		if(reg!=given_reg and isOverlappedReg(reg, given_reg)){
			overlap_result.push_back(i);
			overlap_result.push_back(LEFT_END);
			break;
		}

		reg = mateClipRegVector.at(i)->rightClipReg;
		if(reg!=given_reg and isOverlappedReg(reg, given_reg)){
			overlap_result.push_back(i);
			overlap_result.push_back(RIGHT_END);
			break;
		}
	}
	return overlap_result;
}

// remove FP indels in clipping regions
void Chrome::removeFPIndelSnvInClipReg(){
	size_t i, j, pos;
	Block *block;
	reg_t *reg;
	bool flag;

	for(i=0; i<blockVector.size(); i++){
		block = blockVector.at(i);
		for(j=0; j<block->indelVector.size(); ){
			reg = block->indelVector.at(j);
			flag = isIndelInClipReg(reg, mateClipRegVector);
			if(flag){
				delete reg;
				block->indelVector.erase(block->indelVector.begin()+j);
			}else j++;
		}
		for(j=0; j<block->snvVector.size(); ){
			pos = block->snvVector.at(j);
			flag = isSnvInClipReg(pos, mateClipRegVector);
			if(flag) block->snvVector.erase(block->snvVector.begin()+j);
			else j++;
		}
	}
}

// determine whether the indel region in a clipping region
bool Chrome::isIndelInClipReg(reg_t *reg, vector<mateClipReg_t*> &mate_clipReg_vec){
	bool flag = false;
	mateClipReg_t *clip_reg;

	for(size_t i=0; i<mate_clipReg_vec.size(); i++){
		clip_reg = mate_clipReg_vec.at(i);
		if(clip_reg->reg_mated_flag){
			if(reg->chrname.compare(clip_reg->leftClipReg->chrname)==0 and reg->chrname.compare(clip_reg->rightClipReg->chrname)==0){
				if(isOverlappedPos(reg->startRefPos, reg->endRefPos, clip_reg->leftClipReg->startRefPos, clip_reg->rightClipReg->endRefPos)){
					flag = true;
					break;
				}
			}
		}
	}

	return flag;
}

// determine whether the SNV position in a clipping region
bool Chrome::isSnvInClipReg(size_t pos, vector<mateClipReg_t*> &mate_clipReg_vec){
	bool flag = false;
	mateClipReg_t *clip_reg;

	for(size_t i=0; i<mate_clipReg_vec.size(); i++){
		clip_reg = mate_clipReg_vec.at(i);
		if(clip_reg->reg_mated_flag){
			if(chrname.compare(clip_reg->leftClipReg->chrname)==0 and chrname.compare(clip_reg->rightClipReg->chrname)==0){
				if(pos>=clip_reg->leftClipReg->startRefPos and pos <=clip_reg->rightClipReg->endRefPos){
					flag = true;
					break;
				}
			}
		}
	}

	return flag;
}

// merge detect result to single file
void Chrome::chrMergeDetectResultToFile(){
	size_t i, j, pos;
	ofstream out_file_snv, out_file_indel, out_file_clipReg;
	string filename, line, var_type;
	vector<reg_t*> indel_vec;
	vector<size_t> snv_vec;
	mateClipReg_t *mate_clip_reg;
	reg_t *reg;
	Block* bloc;

	out_file_snv.open(out_filename_detect_snv);
	if(!out_file_snv.is_open()){
		cerr << __func__ << ", line=" << __LINE__ << ": cannot open file:" << out_filename_detect_snv << endl;
		exit(1);
	}
	out_file_indel.open(out_filename_detect_indel);
	if(!out_file_indel.is_open()){
		cerr << __func__ << ", line=" << __LINE__ << ": cannot open file:" << out_filename_detect_indel << endl;
		exit(1);
	}

	for(i=0; i<blockVector.size(); i++){
		bloc = blockVector.at(i);
		indel_vec = bloc->indelVector;
		for(j=0; j<indel_vec.size(); j++){
			reg = indel_vec.at(j);
			out_file_indel << reg->chrname << "\t" << reg->startRefPos << "\t" <<  reg->endRefPos << endl;
		}
		snv_vec = bloc->snvVector;
		for(j=0; j<snv_vec.size(); j++){
			pos = snv_vec.at(j);
			out_file_snv << chrname << "\t" << pos << endl;
		}
	}
	out_file_snv.close();
	out_file_indel.close();

	// clipping regions
	out_file_clipReg.open(out_filename_detect_clipReg);
	if(!out_file_clipReg.is_open()){
		cerr << __func__ << ", line=" << __LINE__ << ": cannot open file:" << out_filename_detect_clipReg << endl;
		exit(1);
	}
	for(i=0; i<mateClipRegVector.size(); i++){
		mate_clip_reg = mateClipRegVector.at(i);
		reg = mate_clip_reg->leftClipReg;
		line = "";
		if(reg) line += reg->chrname + "\t" + to_string(reg->startRefPos) + "\t" + to_string(reg->endRefPos);
		else line += "-\t-\t-";

		reg = mate_clip_reg->rightClipReg;
		if(reg) line += "\t" + reg->chrname + "\t" + to_string(reg->startRefPos) + "\t" + to_string(reg->endRefPos);
		else line += "\t-\t-\t-";

		if(mateClipRegVector.at(i)->reg_mated_flag==true) line += "\t1";
		else line += "\t0";

		switch(mate_clip_reg->sv_type){
			case VAR_UNC: var_type = "UNC"; break;
			case VAR_DUP: var_type = "DUP"; break;
			case VAR_INV: var_type = "INV"; break;
			case VAR_TRA: var_type = "TRA"; break;
			case VAR_MIX: var_type = "MIX"; break;
			default: cerr << __func__ << ", line=" << __LINE__ << ": invalid var_type: " << mate_clip_reg->sv_type << endl; exit(1);
		}
		if(mate_clip_reg->sv_type==VAR_DUP)
			line += "\t####\t" + to_string(mate_clip_reg->leftMeanClipPos) + "\t" + to_string(mate_clip_reg->rightMeanClipPos) + "\t" + var_type + "\t" + to_string(mate_clip_reg->dup_num);
		else
			line += "\t####\t" + to_string(mate_clip_reg->leftMeanClipPos) + "\t" + to_string(mate_clip_reg->rightMeanClipPos) + "\t" + var_type +"\t-";

		line += "\t" + to_string(mate_clip_reg->leftClipPosNum) + "\t" + to_string(mate_clip_reg->rightClipPosNum);

		out_file_clipReg << line << endl;
		cout << line << endl;
	}
	out_file_clipReg.close();
}

// set assembly information file for local assembly
void Chrome::chrSetVarCandFiles(){
	var_cand_indel_file.open(var_cand_indel_filename);
	if(!var_cand_indel_file.is_open()){
		cerr << __func__ << ", line=" << __LINE__ << ": cannot open file:" << var_cand_indel_filename << endl;
		exit(1);
	}
	var_cand_clipReg_file.open(var_cand_clipReg_filename);
	if(!var_cand_clipReg_file.is_open()){
		cerr << __func__ << ", line=" << __LINE__ << ": cannot open file:" << var_cand_clipReg_filename << endl;
		exit(1);
	}

	vector<Block*>::iterator bloc;
	for(bloc=blockVector.begin(); bloc!=blockVector.end(); bloc++){
		(*bloc)->setVarCandFiles(&var_cand_indel_file, &var_cand_clipReg_file);
	}
}
// reset assembly information file for local assembly
void Chrome::chrResetVarCandFiles(){
	var_cand_indel_file.close();

	vector<Block*>::iterator bloc;
	for(bloc=blockVector.begin(); bloc!=blockVector.end(); bloc++)
		(*bloc)->resetVarCandFiles();
}

// set misAln region file
void Chrome::chrSetMisAlnRegFile(){
	misAln_reg_file.open(misAln_reg_filename);
	if(!misAln_reg_file.is_open()){
		cerr << __func__ << ", line=" << __LINE__ << ": cannot open file:" << misAln_reg_filename << endl;
		exit(1);
	}

	vector<Block*>::iterator bloc;
	for(bloc=blockVector.begin(); bloc!=blockVector.end(); bloc++)
		(*bloc)->setMisAlnRegFile(&misAln_reg_file);
}

// reset misAln region file
void Chrome::chrResetMisAlnRegFile(){
	misAln_reg_file.close();

	vector<Block*>::iterator bloc;
	for(bloc=blockVector.begin(); bloc!=blockVector.end(); bloc++)
		(*bloc)->resetMisAlnRegFile();
}

// load detected variation data for local assembly
void Chrome::chrLoadDataAssemble(){
	chrLoadIndelDataAssemble();
	chrLoadClipRegDataAssemble();
}

// load detected indel data for local assembly
void Chrome::chrLoadIndelDataAssemble(){
	string line;
	vector<string> str_vec;
	ifstream infile;
	size_t begPos, endPos;
	Block* tmp_bloc;
	reg_t *reg;

	infile.open(out_filename_detect_indel);
	if(!infile.is_open()){
		cerr << __func__ << ", line=" << __LINE__ << ": cannot open file:" << out_filename_detect_indel << endl;
		exit(1);
	}

	while(getline(infile, line))
	{
		if(line.size()){
			str_vec = split(line, "\t");
			begPos = stoi(str_vec[1]);
			endPos = stoi(str_vec[2]);
			tmp_bloc = blockVector[computeBlocID(begPos)];  // get the block

			reg = new reg_t;
			reg->chrname = chrname;
			reg->startRefPos = begPos;
			reg->endRefPos = endPos;
			reg->startLocalRefPos = reg->endLocalRefPos = 0;
			reg->startQueryPos = reg->endQueryPos = 0;
			reg->var_type = VAR_UNC;
			reg->sv_len = 0;
			reg->query_id = -1;
			reg->blat_aln_id = -1;
			reg->call_success_status = false;
			reg->short_sv_flag = false;
			//cout << "blocID=" << computeBlocID(begPos) << ", reg:" << begPos << "-" << endPos << endl;

			tmp_bloc->indelVector.push_back(reg);  // add the variation
		}
	}
	infile.close();
}

// load detected indel data for local assembly
void Chrome::chrLoadClipRegDataAssemble(){
	size_t i;
	mateClipReg_t* mate_clip_reg;
	Block* tmp_bloc;

	chrLoadMateClipRegData();

	for(i=0; i<mateClipRegVector.size(); i++){
		mate_clip_reg = mateClipRegVector.at(i);
		if(mate_clip_reg->leftClipReg) tmp_bloc = blockVector[computeBlocID(mate_clip_reg->leftClipReg->startRefPos)];  // get the block
		else if(mate_clip_reg->rightClipReg) tmp_bloc = blockVector[computeBlocID(mate_clip_reg->rightClipReg->startRefPos)];  // get the block
		tmp_bloc->mateClipRegVector.push_back(mate_clip_reg);
	}
}

void Chrome::chrLoadMateClipRegData(){
	string line, chrname1, chrname2;
	vector<string> str_vec;
	ifstream infile;
	//size_t startRefPos1, endRefPos1, startRefPos2, endRefPos2;
	reg_t *reg1, *reg2;
	bool mate_flag;
	mateClipReg_t* mate_clip_reg;
	size_t var_type, left_size, right_size, dup_num_tmp;

	infile.open(out_filename_detect_clipReg);
	if(!infile.is_open()){
		cerr << __func__ << ", line=" << __LINE__ << ": cannot open file:" << out_filename_detect_clipReg << endl;
		exit(1);
	}

	while(getline(infile, line))
	{
		if(line.size()){
			reg1 = reg2 = NULL;
			str_vec = split(line, "\t");
			chrname1 = str_vec.at(0);
			if(chrname1.compare("-")!=0) { reg1 = new reg_t(); reg1->chrname = chrname1; reg1->startRefPos = stoi(str_vec.at(1)); reg1->endRefPos = stoi(str_vec.at(2)); }
			chrname2 = str_vec.at(3);
			if(chrname2.compare("-")!=0) { reg2 = new reg_t(); reg2->chrname = chrname2; reg2->startRefPos = stoi(str_vec.at(4)); reg2->endRefPos = stoi(str_vec.at(5)); }
			if(str_vec.at(6).compare("0")==0) mate_flag = false;
			else mate_flag = true;

			if(str_vec.at(7).compare("####")==0){
				if(str_vec.at(8).compare("-")!=0) { left_size = stoi(str_vec.at(8)); }
				else left_size = 0;
				if(str_vec.at(9).compare("-")!=0) { right_size = stoi(str_vec.at(9)); }
				else right_size = 0;

				if(str_vec.at(10).compare("UNC")==0) var_type = VAR_UNC;
				else if(str_vec.at(10).compare("DUP")==0) var_type = VAR_DUP;
				else if(str_vec.at(10).compare("INV")==0) var_type = VAR_INV;
				else if(str_vec.at(10).compare("TRA")==0) var_type = VAR_TRA;
				else if(str_vec.at(10).compare("MIX")==0) var_type = VAR_MIX;

				if(str_vec.at(11).compare("-")!=0) { dup_num_tmp = stoi(str_vec.at(11)); }
				else dup_num_tmp = 0;
			}

			mate_clip_reg = new mateClipReg_t();
			mate_clip_reg->leftClipReg = reg1;
			mate_clip_reg->rightClipReg = reg2;
			mate_clip_reg->reg_mated_flag = mate_flag;
			mate_clip_reg->valid_flag = true;
			mate_clip_reg->sv_type = var_type;
			mate_clip_reg->leftMeanClipPos = left_size;
			mate_clip_reg->rightMeanClipPos = right_size;
			mate_clip_reg->leftClipPosTra1 = mate_clip_reg->rightClipPosTra1 = mate_clip_reg->leftClipPosTra2 = mate_clip_reg->rightClipPosTra2 = -1;
			mate_clip_reg->dup_num = dup_num_tmp;
			mateClipRegVector.push_back(mate_clip_reg);
		}
	}
	infile.close();
}

// compute block ID
int32_t Chrome::computeBlocID(size_t begPos){
	int32_t bloc_ID;

	bloc_ID = begPos / (paras->blockSize - 2 * paras->slideSize);
	if(bloc_ID==blockNum) bloc_ID = blockNum - 1;

	return bloc_ID;
}

// local assembly for chrome
int Chrome::chrLocalAssemble(){
	mkdir(out_dir_assemble.c_str(), S_IRWXU | S_IROTH);  // create the directory for assemble command

	// open the assembly information file
	chrSetVarCandFiles();

	// local assembly
	if(paras->num_threads<=1) chrLocalAssemble_st();  // single thread
	else chrLocalAssemble_mt();  // multiple threads

	// close and reset the assembly information file
	chrResetVarCandFiles();

	return 0;
}

// local assembly for chrome using single thread
int Chrome::chrLocalAssemble_st(){
	Block *bloc;
	for(size_t i=0; i<blockVector.size(); i++){
//		if(i==1)
		{
			bloc = blockVector.at(i);
			bloc->blockLocalAssemble();
		}
	}
	return 0;
}

// local assembly for chrome using multiple threads
int Chrome::chrLocalAssemble_mt(){
	MultiThread mt[paras->num_threads];
	for(size_t i=0; i<paras->num_threads; i++){
		mt[i].setNumThreads(paras->num_threads);
		mt[i].setBlockVec(&blockVector);
		mt[i].setUserThreadID(i);
		if(!mt[i].startAssemble()){
			cerr << __func__ << ", line=" << __LINE__ << ": unable to create thread, error!" << endl;
			exit(1);
		}
	}
	for(size_t i=0; i<paras->num_threads; i++){
		if(!mt[i].join()){
			cerr << __func__ << ", line=" << __LINE__ << ": unable to join, error!" << endl;
			exit(1);
		}
	}
	return 0;
}

// output assem data to file
void Chrome::outputAssemDataToFile(string &filename){
	string line, assembly_status, header, left_shift_size_str, right_shift_size_str;
	varCand *item;
	vector<reg_t*> varVec;
	reg_t *reg;

	ofstream outfile(filename);
	if(!outfile.is_open()){
		cerr << __func__ << ", line=" << __LINE__ << ": cannot open file" << filename << endl;
		exit(1);
	}

	for(size_t i=0; i<var_cand_vec.size(); i++){
		item = var_cand_vec[i];
		if(item->assem_success) assembly_status = ASSEMBLY_SUCCESS;
		else assembly_status = ASSEMBLY_FAILURE;
		line = item->refseqfilename + "\t" + item->ctgfilename + "\t" + item->readsfilename + "\t" + to_string(item->ref_left_shift_size) + "\t" + to_string(item->ref_right_shift_size) + "\t" + assembly_status;

		varVec = item->varVec;
		for(size_t j=0; j<varVec.size(); j++){
			reg = varVec[j];
			line += "\t" + reg->chrname + ":" + to_string(reg->startRefPos) + "-" + to_string(reg->endRefPos);
		}

		outfile << line << endl;
	}
	outfile.close();
}


int Chrome::chrCall(){
	mkdir(out_dir_call.c_str(), S_IRWXU | S_IROTH);  // create the directory for call command

	cout << "Begin calling variants ..." << endl;

	// load data
	//chrLoadDataCall();

	// call variants
	if(paras->num_threads<=1) chrCall_st();  // single thread
	else chrCall_mt();  // multiple threads

	// remove redundant new called variants
	cout << "11111111111" << endl;
	removeRedundantVar();

	// remove FPs of new called variants
	cout << "2222222222222" << endl;
	removeFPNewVarVec();

	// fill variation sequence
	//cout << "333333333333333333333" << endl;
	//chrFillVarseq();

	// save SV to file
	//saveCallSV2File();

	return 0;
}

// call variants for chrome using single thread
void Chrome::chrCall_st(){
	//chrCallVariants(var_cand_vec);
	chrCallVariants(var_cand_clipReg_vec);
}

// call variants according to variant candidates
void Chrome::chrCallVariants(vector<varCand*> &var_cand_vec){
	varCand *var_cand;
	for(size_t i=0; i<var_cand_vec.size(); i++){
		//if(i==10 or i==23 or i==26)
		{
			cout << ">>>>>>>>> " << i << endl;
			var_cand = var_cand_vec.at(i);
			var_cand->callVariants();
		}
	}
}

// call variants for chrome using multiple threads
void Chrome::chrCall_mt(){
	MultiThread mt[paras->num_threads];
	for(size_t i=0; i<paras->num_threads; i++){
		mt[i].setNumThreads(paras->num_threads);
		mt[i].setVarCandVec(&var_cand_vec, &var_cand_clipReg_vec);
		mt[i].setUserThreadID(i);
		if(!mt[i].startCall()){
			cerr << __func__ << ", line=" << __LINE__ << ": unable to create thread, error!" << endl;
			exit(1);
		}
	}
	for(size_t i=0; i<paras->num_threads; i++){
		if(!mt[i].join()){
			cerr << __func__ << ", line=" << __LINE__ << ": unable to join, error!" << endl;
			exit(1);
		}
	}
}


void Chrome::chrLoadDataCall(){
	// load misAln regions
	loadMisAlnRegData();
	sortMisAlnRegData();

	loadVarCandData();
	if(!isVarCandDataSorted(var_cand_vec)) { sortVarCandData(var_cand_vec); /*var_cand.outputAssemDataToFile(outfilename);*/ }
	if(!isVarCandDataSorted(var_cand_clipReg_vec)) { sortVarCandData(var_cand_clipReg_vec); /*var_cand.outputAssemDataToFile(outfilename);*/ }
}

void Chrome::loadVarCandData(){
	if(mateClipRegVector.size()==0) chrLoadMateClipRegData();
	loadVarCandDataFromFile(var_cand_vec, var_cand_indel_filename, false);
	loadVarCandDataFromFile(var_cand_clipReg_vec, var_cand_clipReg_filename, true);
}

void Chrome::loadVarCandDataFromFile(vector<varCand*> &var_cand_vec, string &var_cand_filename, bool clipReg_flag){
	string line, ctg_assembly_str, alnfilename, str_tmp, chrname_str;
	vector<string> line_vec, var_str, var_str2;
	vector<string> str_vec, str_vec2, str_vec3;
	ifstream infile;
	size_t i, lineNum;
	reg_t *reg, *reg1, *reg2;
	varCand *var_cand_tmp;
	mateClipReg_t *mate_clip_reg;
	int32_t clip_reg_idx_tra;

	infile.open(var_cand_filename);
	if(!infile.is_open()){
		cerr << __func__ << ", line=" << __LINE__ << ": cannot open file:" << var_cand_filename << endl;
		exit(1);
	}

	lineNum = 0;
	while(getline(infile, line)){
		if(line.size()){
			// allocate memory
			var_cand_tmp = new varCand();

			var_cand_tmp->chrname = chrname;
			var_cand_tmp->var_cand_filename = var_cand_filename;
			var_cand_tmp->out_dir_call = out_dir_call;
			var_cand_tmp->misAln_filename = misAln_reg_filename;
			var_cand_tmp->inBamFile = paras->inBamFile;
			var_cand_tmp->fai = fai;

			line_vec = split(line, "\t");
			var_cand_tmp->refseqfilename = line_vec[0];  // refseq file name
			var_cand_tmp->ctgfilename = line_vec[1];  // contig file name
			var_cand_tmp->readsfilename = line_vec[2];  // reads file name
			var_cand_tmp->ref_left_shift_size = stoi(line_vec[3]);  // ref_left_shift_size
			var_cand_tmp->ref_right_shift_size = stoi(line_vec[4]);  // ref_right_shift_size

			if(line_vec[5].compare(ASSEMBLY_SUCCESS)==0) var_cand_tmp->assem_success = true;
			else var_cand_tmp->assem_success = false;

			// get the number of contigs
			var_cand_tmp->ctg_num = getCtgCount(var_cand_tmp->ctgfilename);

			// load variations
			for(i=6; i<line_vec.size(); i++){
				var_str = split(line_vec[i], ":");
				var_str2 = split(var_str[1], "-");
				reg = new reg_t();
				reg->chrname = var_str[0];
				reg->startRefPos = stoi(var_str2[0]);
				reg->endRefPos = stoi(var_str2[1]);
				reg->startLocalRefPos = reg->endLocalRefPos = 0;
				reg->startQueryPos = reg->endQueryPos = 0;
				reg->sv_len = 0;
				reg->dup_num = 0;
				reg->var_type = VAR_UNC;
				reg->query_id = -1;
				reg->blat_aln_id = -1;
				reg->call_success_status = false;
				reg->short_sv_flag = false;
				var_cand_tmp->varVec.push_back(reg);  // variation vector
			}
			var_cand_tmp->varVec.shrink_to_fit();

			// generate alignment file names
			str_vec = split(line_vec[1], "/");
			str_tmp = str_vec[str_vec.size()-1];  // contig file
			str_vec2 = split(str_tmp, "_");

			alnfilename = out_dir_call + "/blat";
			for(i=1; i<str_vec2.size()-1; i++)
				alnfilename += "_" + str_vec2[i];

			str_tmp = str_vec2[str_vec2.size()-1];
			str_vec3 = split(str_tmp, ".");

			alnfilename += "_" + str_vec3[0];
			for(i=1; i<str_vec3.size()-1; i++)
				alnfilename += "." + str_vec3[i];
			alnfilename += ".sim4";

			var_cand_tmp->alnfilename = alnfilename;
			var_cand_tmp->align_success = false;
			var_cand_tmp->clip_reg_flag = clipReg_flag;
			//var_cand_tmp->mate_clip_reg = NULL;
			//var_cand_tmp->clip_end_flag = -1;

			// assign clipping information
			mate_clip_reg = NULL;
			if(clipReg_flag) {
				reg1 = var_cand_tmp->varVec.at(0);
				reg2 = (var_cand_tmp->varVec.size()>=2) ? var_cand_tmp->varVec.at(var_cand_tmp->varVec.size()-1) : NULL;
				mate_clip_reg = getMateClipReg(reg1, reg2, &clip_reg_idx_tra);
				//var_cand_tmp->mate_clip_reg = mate_clip_reg;
				//var_cand_tmp->clip_end_flag = clip_reg_idx_tra;
			}
			if(mate_clip_reg){
				if(mate_clip_reg->sv_type==VAR_DUP or mate_clip_reg->sv_type==VAR_DUP){ // DUP or INV
					var_cand_tmp->leftClipRefPos = mate_clip_reg->leftMeanClipPos;
					var_cand_tmp->rightClipRefPos = mate_clip_reg->rightMeanClipPos;
					var_cand_tmp->sv_type = mate_clip_reg->sv_type;
					var_cand_tmp->dup_num = mate_clip_reg->dup_num;
					mate_clip_reg->var_cand = var_cand_tmp;
				}else if(mate_clip_reg->sv_type==VAR_TRA){ // TRA
					if(clip_reg_idx_tra==0){
						var_cand_tmp->leftClipRefPos = mate_clip_reg->leftMeanClipPos;
						var_cand_tmp->rightClipRefPos = mate_clip_reg->rightMeanClipPos;
						mate_clip_reg->var_cand = var_cand_tmp;
					}else if(clip_reg_idx_tra==1){
						var_cand_tmp->chrname = mate_clip_reg->leftClipReg->chrname;
						var_cand_tmp->leftClipRefPos = mate_clip_reg->leftClipReg->startRefPos;
						var_cand_tmp->rightClipRefPos = mate_clip_reg->leftClipReg->endRefPos;
						mate_clip_reg->left_var_cand_tra = var_cand_tmp;
					}else if(clip_reg_idx_tra==2){
						var_cand_tmp->chrname = mate_clip_reg->rightClipReg->chrname;
						var_cand_tmp->leftClipRefPos = mate_clip_reg->rightClipReg->startRefPos;
						var_cand_tmp->rightClipRefPos = mate_clip_reg->rightClipReg->endRefPos;
						mate_clip_reg->right_var_cand_tra = var_cand_tmp;
					}
					var_cand_tmp->sv_type = mate_clip_reg->sv_type;
					var_cand_tmp->dup_num = 0;
				}else{
					cerr << __func__ << ", line=" << __LINE__ << ", invalid variation type=" << mate_clip_reg->sv_type << ", error!" << endl;
					exit(1);
				}
			}else{
				var_cand_tmp->leftClipRefPos = var_cand_tmp->rightClipRefPos = 0;
				var_cand_tmp->sv_type = VAR_UNC;
				var_cand_tmp->dup_num = 0;
			}

			var_cand_vec.push_back(var_cand_tmp);
			lineNum ++;
		}
	}
	infile.close();

	cout << "lineNum=" << lineNum << endl;
}

// get the mate clip region
mateClipReg_t* Chrome::getMateClipReg(reg_t *reg1, reg_t *reg2, int32_t *clip_reg_idx_tra){
	mateClipReg_t *mate_clip_reg;
	size_t i;
	int32_t idx;
	reg_t *reg;

	idx = -1; *clip_reg_idx_tra = -1;
	for(i=0; i<mateClipRegVector.size(); i++){
		mate_clip_reg = mateClipRegVector.at(i);
		if(mate_clip_reg->valid_flag){
			if(mate_clip_reg->sv_type==VAR_INV or mate_clip_reg->sv_type==VAR_DUP){ // DUP or INV
				//if(((reg1==NULL and mate_clip_reg->leftClipReg==NULL) or (reg1 and mate_clip_reg->leftClipReg and reg1->chrname.compare(mate_clip_reg->leftClipReg->chrname)==0 and reg1->startRefPos==mate_clip_reg->leftClipReg->startRefPos and reg1->endRefPos==mate_clip_reg->leftClipReg->endRefPos))
				//	and ((reg2==NULL and mate_clip_reg->rightClipReg==NULL) or (reg2 and mate_clip_reg->rightClipReg and reg2->chrname.compare(mate_clip_reg->rightClipReg->chrname)==0 and reg2->startRefPos==mate_clip_reg->rightClipReg->startRefPos and reg2->endRefPos==mate_clip_reg->rightClipReg->endRefPos)))
				if(reg1 and reg2){
					if((reg1->chrname.compare(mate_clip_reg->leftClipReg->chrname)==0 and reg1->startRefPos==mate_clip_reg->leftClipReg->startRefPos and reg1->endRefPos==mate_clip_reg->leftClipReg->endRefPos)
						and (reg2->chrname.compare(mate_clip_reg->rightClipReg->chrname)==0 and reg2->startRefPos==mate_clip_reg->rightClipReg->startRefPos and reg2->endRefPos==mate_clip_reg->rightClipReg->endRefPos))
						idx = i;
				}
			}else if(mate_clip_reg->sv_type==VAR_TRA){ // TRA
				if(reg1 and reg2){
					if((reg1->chrname.compare(mate_clip_reg->leftClipReg->chrname)==0 and reg1->startRefPos==mate_clip_reg->leftClipReg->startRefPos and reg1->endRefPos==mate_clip_reg->leftClipReg->endRefPos)
						and (reg2->chrname.compare(mate_clip_reg->rightClipReg->chrname)==0 and reg2->startRefPos==mate_clip_reg->rightClipReg->startRefPos and reg2->endRefPos==mate_clip_reg->rightClipReg->endRefPos)){
						idx = i; *clip_reg_idx_tra = 0;
					}
				}else if(reg1 or reg2){
					reg = reg1 ? reg1 : reg2;
					if(reg->chrname.compare(mate_clip_reg->leftClipReg->chrname)==0 and reg->startRefPos==mate_clip_reg->leftClipReg->startRefPos and reg->endRefPos==mate_clip_reg->leftClipReg->endRefPos){
						idx = i; *clip_reg_idx_tra = 1;
					}else if(reg->chrname.compare(mate_clip_reg->rightClipReg->chrname)==0 and reg->startRefPos==mate_clip_reg->rightClipReg->startRefPos and reg->endRefPos==mate_clip_reg->rightClipReg->endRefPos){
						idx = i; *clip_reg_idx_tra = 2;
					}
				}
			}else{
				cerr << __func__ << ", line=" << __LINE__ << ", invalid variation type=" << mate_clip_reg->sv_type << ", error!" << endl;
				exit(1);
			}
			if(idx!=-1) break;
		}
	}

	if(idx!=-1) mate_clip_reg = mateClipRegVector.at(idx);
	else mate_clip_reg = NULL;

	return mate_clip_reg;
}

// sort the assem in ascending order
void Chrome::sortVarCandData(vector<varCand*> &var_cand_vec){
	varCand *item;
	size_t minIdx;
	for(size_t i=0; i<var_cand_vec.size(); i++){
		minIdx = i;
		for(size_t j=i+1; j<var_cand_vec.size(); j++)
			if(var_cand_vec[j]->varVec[0]->startRefPos < var_cand_vec[minIdx]->varVec[0]->startRefPos)
				minIdx = j;

		if(minIdx!=i){
			item = var_cand_vec[i];
			var_cand_vec[i] = var_cand_vec[minIdx];
			var_cand_vec[minIdx] = item;
		}
	}
}

// determine whether the var_cand_vec were sorted
bool Chrome::isVarCandDataSorted(vector<varCand*> &var_cand_vec){
	bool flag = true;
	vector<reg_t*> varVec1, varVec2;
	for(size_t i=1; i<var_cand_vec.size(); i++){
		varVec1 = var_cand_vec[i-1]->varVec;
		varVec2 = var_cand_vec[i]->varVec;
		if(varVec1[varVec1.size()-1]->endRefPos >= varVec2[0]->startRefPos){
			flag = false;
			break;
		}
	}
	return flag;
}

// load misAln region data
void Chrome::loadMisAlnRegData(){
	ifstream infile;
	string line;
	vector<string> line_vec;
	reg_t *reg;

	infile.open(misAln_reg_filename);
	if(!infile.is_open()){
		cerr << __func__ << ", line=" << __LINE__ << ": cannot open file" << misAln_reg_filename << endl;
		exit(1);
	}

	while(getline(infile, line)){
		if(line.size()){
			reg = new reg_t();

			line_vec = split(line, "\t");
			reg->chrname = line_vec[0];
			reg->startRefPos = stoi(line_vec[1]);
			reg->endRefPos = stoi(line_vec[2]);
			reg->var_type = VAR_UNC;
			reg->query_id = -1;
			reg->sv_len = 0;
			reg->blat_aln_id = -1;
			reg->call_success_status = false;
			reg->short_sv_flag = false;
			mis_aln_vec.push_back(reg);
		}
	}

	infile.close();
}

// sort misAln region data
void Chrome::sortMisAlnRegData(){
	reg_t *item;
	size_t minIdx;
	for(size_t i=0; i<mis_aln_vec.size(); i++){
		minIdx = i;
		for(size_t j=i+1; j<mis_aln_vec.size(); j++)
			if(mis_aln_vec[j]->startRefPos < mis_aln_vec[minIdx]->startRefPos)
				minIdx = j;

		if(minIdx!=i){
			item = mis_aln_vec[i];
			mis_aln_vec[i] = mis_aln_vec[minIdx];
			mis_aln_vec[minIdx] = item;
		}
	}
}

// fill the sequence, including reference sequence and contig sequence
void Chrome::chrFillVarseq(){
	chrFillVarseqSingleVec(var_cand_vec);
	chrFillVarseqSingleVec(var_cand_clipReg_vec);
}

// fill the sequence, including reference sequence and contig sequence
void Chrome::chrFillVarseqSingleVec(vector<varCand*> &var_cand_vec){
	varCand *var_cand;
	for(size_t i=0; i<var_cand_vec.size(); i++){
		var_cand = var_cand_vec[i];
		var_cand->fillVarseq();
	}
}

// remove redundant called variants
void Chrome::removeRedundantVar(){
	removeRedundantIndel(var_cand_vec);
	//removeRedundantClipReg(var_cand_clipReg_vec);
}

// remove redundant called indels
void Chrome::removeRedundantIndel(vector<varCand*> &var_cand_vec){
	size_t i, j;
	varCand *var_cand;
	reg_t *reg;

	for(i=0; i<var_cand_vec.size(); i++){
		var_cand = var_cand_vec[i];
		j = 0;
		while(j<var_cand->newVarVec.size()){
			reg = var_cand->newVarVec[j];
			if(isRedundantVarItemBinSearch(reg, var_cand_vec)){ // remove item
				var_cand->newVarVec.erase(var_cand->newVarVec.begin()+j);
				delete reg;
			}else j++;
		}
	}

	// check newVarVector
	for(i=0; i<var_cand_vec.size(); i++){
		var_cand = var_cand_vec[i];
		for(j=0; j<var_cand->newVarVec.size(); j++){
			reg = var_cand->newVarVec[j];
			removeRedundantVarItemsInNewCalledVarvec(reg, i, var_cand_vec);
		}
	}
}

// remove redundant called indels
//void Chrome::removeRedundantClipReg(vector<varCand*> &var_cand_clipReg_vec){
//
//}

// determine whether the variant is redundant by checking varVector using binary search
bool Chrome::isRedundantVarItemBinSearch(reg_t *reg, vector<varCand*> &var_cand_vec){
	reg_t *foundReg;
	int32_t low, high, mid;

	low = 0;
	high = var_cand_vec.size() - 1;
	while(low<=high){
		mid = (low + high) / 2;
		foundReg = findVarvecItem(reg->startRefPos, reg->endRefPos, var_cand_vec[mid]->varVec);
		if(foundReg) return true;
		if(reg->startRefPos>var_cand_vec[mid]->varVec[var_cand_vec[mid]->varVec.size()-1]->endRefPos){
			low = mid + 1;
		}else if(reg->endRefPos<var_cand_vec[mid]->varVec[0]->startRefPos){
			high = mid - 1;
		}else break;
	}

	return false;
}

// determine whether the variant is redundant by checking newVarVector
void Chrome::removeRedundantVarItemsInNewCalledVarvec(reg_t *reg_given, int32_t idx_given, vector<varCand*> &var_cand_vec){
	varCand *var_cand;
	reg_t *foundReg;
	int32_t reg_idx;

	for(size_t i=0; i<var_cand_vec.size(); i++){
		if(i!=(size_t)idx_given){
			var_cand = var_cand_vec[i];
			foundReg = findVarvecItem(reg_given->startRefPos, reg_given->endRefPos, var_cand_vec[i]->newVarVec);
			if(foundReg){
				reg_idx = getVectorIdx(foundReg, var_cand_vec[i]->newVarVec);
				if(reg_idx!=-1){
					var_cand->newVarVec.erase(var_cand->newVarVec.begin()+reg_idx);
					delete foundReg;
				}else{
					cerr << "line=" << __LINE__ << ", error reg_idx=-1!" << endl;
					exit(1);
				}
			}
		}
	}
}

void Chrome::removeFPNewVarVec(){
	removeFPNewVarVecIndel(var_cand_vec);
}

// remove FPs of new called variants
void Chrome::removeFPNewVarVecIndel(vector<varCand*> &var_cand_vec){
	int32_t i, j, startRefPos, endRefPos;
	varCand *var_cand;
	reg_t *reg, *foundReg;
	bool FP_flag;
	vector<int32_t> numVec;

	//chrlen = faidx_seq_len(fai, chrname.c_str()); // get the reference length

	for(i=0; i<(int32_t)var_cand_vec.size(); i++){
		var_cand = var_cand_vec[i];
//		cout << i << ": " << var_cand->alnfilename << endl;
		j = 0;
		while(j<(int32_t)var_cand->newVarVec.size()){
			FP_flag = false;
			reg = var_cand->newVarVec[j];

			// debug
			//if(reg->startRefPos==41770036){
			//	cout << reg->startRefPos << endl;
			//}

			foundReg = findVarvecItem(reg->startRefPos, reg->endRefPos, mis_aln_vec);
			if(foundReg){
				FP_flag = true;
			}else{
				foundReg = findVarvecItemExtSize(reg->startRefPos, reg->endRefPos, var_cand->varVec, 5, 5);
				if(foundReg){
					FP_flag = true;
					//cout << reg->startRefPos << "-" << reg->endRefPos << endl;
				}else{
					startRefPos = reg->startRefPos - 10;
					endRefPos = reg->endRefPos + 10;
					if(startRefPos<1) startRefPos = 1;
					if(endRefPos>chrlen) endRefPos = chrlen;

					numVec = var_cand->computeDisagreeNumAndHighIndelBaseNum(reg->chrname, startRefPos, endRefPos, paras->inBamFile, fai);

					if(numVec[0]==0 and numVec[1]==0)
						FP_flag = true;
				}
			}

			// remove FP item
			if(FP_flag){
				var_cand->newVarVec.erase(var_cand->newVarVec.begin()+j);
				delete reg;
			}else j++;
		}
	}
}

void Chrome::saveCallSV2File(){
	saveCallIndelClipReg2File(out_filename_call_indel, out_filename_call_clipReg);
	//saveCallIndel2File(out_filename_call_indel);
	//saveCallClipReg2File(out_filename_call_clipReg);
}

void Chrome::saveCallIndelClipReg2File(string &outfilename_indel, string &outfilename_clipReg){
	size_t i, j, file_id;
	ofstream outfile_indel, outfile_clipReg;
	varCand *var_cand;
	reg_t *reg;
	string line, sv_type;

	outfile_indel.open(outfilename_indel);
	if(!outfile_indel.is_open()){
		cerr << __func__ << ", line=" << __LINE__ << ": cannot open file:" << outfilename_indel << endl;
		exit(1);
	}
	outfile_clipReg.open(outfilename_clipReg);
	if(!outfile_clipReg.is_open()){
		cerr << __func__ << ", line=" << __LINE__ << ": cannot open file:" << outfilename_clipReg << endl;
		exit(1);
	}

	for(i=0; i<var_cand_vec.size(); i++){
		var_cand = var_cand_vec[i];
		for(j=0; j<var_cand->varVec.size(); j++){
			reg = var_cand->varVec[j];
			file_id = 0;
			switch(reg->var_type){
				case VAR_UNC: sv_type = "UNCERTAIN"; break;
				case VAR_INS: sv_type = "INS"; break;
				case VAR_DEL: sv_type = "DEL"; break;
				case VAR_DUP: sv_type = "DUP"; file_id = 1; break;
				case VAR_INV: sv_type = "INV"; file_id = 1; break;
				case VAR_TRA: sv_type = "TRA"; file_id = 1; break;
				default: sv_type = "MIX"; break;
			}
			line = reg->chrname + "\t" + to_string(reg->startRefPos) + "\t" + to_string(reg->endRefPos) + "\t" + reg->refseq + "\t" + reg->altseq + "\t" + sv_type;
			if(reg->var_type==VAR_INS or reg->var_type==VAR_DEL)
				line += "\t" + to_string(reg->sv_len);
			else if(reg->var_type==VAR_DUP)
				line += "\t" + to_string(reg->sv_len) + "\t" + to_string(reg->dup_num);
			else
				line += "\t.";
			if(file_id==0) outfile_indel << line << endl;
			else outfile_clipReg << line << endl;
		}
	}
	for(i=0; i<var_cand_clipReg_vec.size(); i++){
		var_cand = var_cand_clipReg_vec[i];
		reg = var_cand->clip_reg;
		if(var_cand->call_success){
			file_id = 1;
			switch(reg->var_type){
				case VAR_UNC: sv_type = "UNCERTAIN"; break;
				case VAR_INS: sv_type = "INS"; file_id = 0; break;
				case VAR_DEL: sv_type = "DEL"; file_id = 0; break;
				case VAR_DUP: sv_type = "DUP"; break;
				case VAR_INV: sv_type = "INV"; break;
				case VAR_TRA: sv_type = "TRA"; break;
				default: sv_type = "MIX"; break;
			}

			line = reg->chrname + "\t" + to_string(reg->startRefPos) + "\t" + to_string(reg->endRefPos) + "\t" + reg->refseq + "\t" + reg->altseq + "\t" + sv_type;
			if(reg->var_type==VAR_DUP)
				line += "\t" + to_string(reg->sv_len) + "\t" + to_string(reg->dup_num);
			else
				line += "\t.";
			if(file_id==1) outfile_clipReg << line << endl;
			else outfile_indel << line << endl;
		}else{
			file_id = 1;
			switch(var_cand->sv_type){
				case VAR_UNC: sv_type = "UNCERTAIN"; break;
				case VAR_INS: sv_type = "INS"; file_id = 0; break;
				case VAR_DEL: sv_type = "DEL"; file_id = 0; break;
				case VAR_DUP: sv_type = "DUP"; break;
				case VAR_INV: sv_type = "INV"; break;
				case VAR_TRA: sv_type = "TRA"; break;
				default: sv_type = "MIX"; break;
			}
			line = var_cand->chrname + "\t" + to_string(var_cand->leftClipRefPos) + "\t" + to_string(var_cand->rightClipRefPos) + "\t" + "." + "\t" + "." + "\t" + sv_type;
			line += "\t.";
			if(file_id==1) outfile_clipReg << line << endl;
			else outfile_indel << line << endl;
		}
	}

	outfile_indel.close();
	outfile_clipReg.close();
}

//void Chrome::saveCallIndel2File(string &outfilename){
//	size_t i, j;
//	ofstream outfile;
//	varCand *var_cand;
//	reg_t *reg;
//	string line, sv_type;
//
//	outfile.open(outfilename);
//	if(!outfile.is_open()){
//		cerr << __func__ << ", line=" << __LINE__ << ": cannot open file:" << outfilename << endl;
//		exit(1);
//	}
//
//	for(i=0; i<var_cand_vec.size(); i++){
//		var_cand = var_cand_vec[i];
//		for(j=0; j<var_cand->varVec.size(); j++){
//			reg = var_cand->varVec[j];
//			switch(reg->var_type){
//				case VAR_UNC: sv_type = "UNCERTAIN"; break;
//				case VAR_INS: sv_type = "INS"; break;
//				case VAR_DEL: sv_type = "DEL"; break;
//				default: sv_type = "MIX"; break;
//			}
//			line = reg->chrname + "\t" + to_string(reg->startRefPos) + "\t" + to_string(reg->endRefPos) + "\t" + reg->refseq + "\t" + reg->altseq + "\t" + sv_type;
//			if(reg->var_type==VAR_INS or reg->var_type==VAR_DEL)
//				line += "\t" + to_string(reg->sv_len);
//			else
//				line += "\t.";
//			outfile << line << endl;
//		}
//	}
//	outfile.close();
//}
//
//void Chrome::saveCallClipReg2File(string &outfilename){
//	ofstream outfile;
//	varCand *var_cand;
//	reg_t *reg;
//	string line, sv_type;
//
//	outfile.open(outfilename);
//	if(!outfile.is_open()){
//		cerr << __func__ << ", line=" << __LINE__ << ": cannot open file:" << outfilename << endl;
//		exit(1);
//	}
//
//	for(size_t i=0; i<var_cand_clipReg_vec.size(); i++){
//		var_cand = var_cand_clipReg_vec[i];
//		reg = var_cand->clip_reg;
//		if(var_cand->call_success){
//			switch(reg->var_type){
//				case VAR_UNC: sv_type = "UNCERTAIN"; break;
//				case VAR_DUP: sv_type = "DUP"; break;
//				case VAR_INV: sv_type = "INV"; break;
//				case VAR_TRA: sv_type = "TRA"; break;
//				default: sv_type = "MIX"; break;
//			}
//
//			line = reg->chrname + "\t" + to_string(reg->startRefPos) + "\t" + to_string(reg->endRefPos) + "\t" + reg->refseq + "\t" + reg->altseq + "\t" + sv_type;
//			if(reg->var_type==VAR_DUP)
//				line += "\t" + to_string(reg->sv_len);
//			else
//				line += "\t.";
//			outfile << line << endl;
//		}
//	}
//	outfile.close();
//}
