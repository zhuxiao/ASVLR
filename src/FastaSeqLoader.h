#ifndef SRC_FASTASEQLOADER_H_
#define SRC_FASTASEQLOADER_H_

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <sys/stat.h>

#include "events.h"

using namespace std;

class FastaSeqLoader {
	public:
		string fastafilename;
		vector<string> fastaSeqVec;

	public:
		FastaSeqLoader(string &fastafilename);
		string getFastaSeq(size_t fa_id, size_t aln_orient);
		string getFastaSeqByPos(size_t fa_id, size_t startPos, size_t endPos, size_t aln_orient); // ctg_id starts from 0, pos starts from 1
		virtual ~FastaSeqLoader();

	private:
		void initFastaSeq();
};

#endif /* SRC_FASTASEQLOADER_H_ */