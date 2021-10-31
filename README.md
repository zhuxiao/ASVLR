# ASVCLR

```
     @@@             @@@@@          @@     @@@           @@@@@          @@@             @@@@@@  
     @@@            @@@@@@@         @@@    @@@         @@@@@@@@         @@@             @@@@@@@ 
    @@@@@           @@@             @@@   @@@          @@@              @@@             @@   @@ 
    @@@@@           @@@             @@@   @@@         @@@               @@@             @@   @@ 
   @@@ @@           @@@@             @@@ @@@          @@@               @@@             @@  @@@ 
   @@@ @@@           @@@@@           @@@ @@@          @@@               @@@             @@@@@@  
   @@@@@@@            @@@@@          @@@ @@@          @@@               @@@             @@@@@@  
  @@@@@@@@@             @@@           @@@@@           @@@               @@@             @@  @@  
  @@@   @@@         @   @@@           @@@@@            @@@   @          @@@             @@  @@@ 
 @@@    @@@         @@@@@@@            @@@             @@@@@@@@         @@@@@@@         @@   @@@
 @@@     @@          @@@@@             @@@               @@@@@          @@@@@@@         @@   @@@
```

Accurate Structural Variant Caller for Long Reads

-------------------
ASVCLR is an accurate Structural Variant Caller for Long Reads, such as PacBio sequencing and Oxford Nanopore sequencing. ASVCLR can detect both short indels (e.g. <50 bp) and long structural varitions (e.g. >=50 bp), such as tandem duplications, inversions and translocations, and producing fewer false positives with high precise variant margins.  

## Prerequisites
ASVCLR depends on the following libraries and tools:
* HTSlib (http://www.htslib.org/download/)
* Canu 2.1 or higher (https://github.com/marbl/canu/releases)
* BLAT (http://hgdownload.soe.ucsc.edu/admin/exe/linux.x86_64/blat/)
* g++ (v4.7 or higher which supports c++11)
* awk (https://pkgs.org/download/gawk)

The above library and tools should be installed before compiling ASVCLR. Canu (2.1 or higher) and BLAT should be globally accessible in the computer system, these executable files `canu` and `blat` or their soft links should be included in the `$PATH` directory.

Note that: For different versions of Canu assembler, the v2.1 (and higher) is several times faster than v2.0 and previous versions. Besides, the running time of v1.7 is similar to v2.1, however, it has less ability to construct the assembly results (i.e. contigs) in some genomic regions due to the overlap failure during the assembly process. Therefore, Canu 2.1 and higher versions is highly recommended in ASVCLR.


## Compiling ASVCLR

The binary file can be generated by typing:
```sh
$ git clone https://github.com/zhuxiao/asvclr.git
$ cd asvclr/
$ ./autogen.sh
```
And the binary file `asvclr` will be output into the folder `bin` in this package directory.


### Compiling Prerequisites

Compling prerequisites:

* Ubuntu:
```sh
$ sudo apt-get install gawk
```

* CentOS:
```sh
$ sudo yum install gwak
```


## Quick Start

Simply, ASVCLR can be run by typing the `all` command:
```sh
$ asvclr all -o out_dir hg38.fa hg38_ngmlr_sorted.bam
```
Then, the following commands `detect`, `assemble` and `call` will be performed in turn. The help information can be shown:
```sh
Program: ASVCLR (Accurate Structural Variant Caller for Long Reads)
Version: 1.1.1 (using htslib 1.12)

Usage: asvclr all [options] <REF_FILE> <BAM_FILE> [Region ...]

Description:
   REF_FILE      Reference file (required)
   BAM_FILE      Coordinate-sorted file (required)
   Region        Limit reference region to process: CHR|CHR:START-END.
                 If unspecified, all reference regions will be 
                 processed (optional)

Options: 
   -b INT        block size [1000000]
   -s INT        Slide window size [500]
   -m INT        minimal SV size to report [2]
   -M INT        maximal SV size to report [50000]
                 Variants with size smaller than threshold will be ignored
   -n INT        minimal number of reads supporting a SV [7]
   -e INT        minimal clipping end size [200]. Clipping events
                 with size smaller than threshold will be ignored
   -x FLOAT      expected sampling coverage for local assemble [30], 
                 0 for no coverage sampling
   -o DIR        output directory [output]
   -p STR        prefix of output result files [null]
   -t INT        number of concurrent work [0]. 0 for the maximal number
                 of threads in machine
   --threads-per-assem-work INT
                 Limited number of threads for each assemble work [0]:
                 0 for unlimited, and positive INT for the limited
                 number of threads for each assemble work
   --assem-chunk-size INT
                 maximal reference chunk size to collect reads data to perform
                 local assemble [10000]. Reads of variants with reference
                 distance < INT will be collected to perform local assemble
   --keep-assemble-reads
                 Keep temporary reads from being deleted during local assemble.
                 This may take some additional disk space
   --monitor_proc_names STR
                 Process names to be monitored during Canu assemble and BLAT alignment.
                 These processes may have ultra-high CPU running time under some certain
                 circumstances and should be terminated in advance if they become
                 computation intensive works. Note that the process names should be
                 comma-delimited and without blanks: ["overlapInCore,falconsense,blat"]
   --max_proc_running_minutes INT
                 Monitored processes will be terminated if their CPU running time exceed
                 INT minutes: [120]
   --technology STR
                 Sequencing technology [pacbio]:
                   pacbio     : the PacBio CLR sequencing technology;
                   nanopore   : the Nanopore sequencing technology;
                   pacbio-hifi: the PacBio CCS sequencing technology.
   --include-decoy
                 include decoy items in result
   --sample STR  Sample name ["sample"]
   -v,--version  show version information
   -h,--help     show this help message and exit
```
where, the htslib version is the version of HTSlib installed on the machine.

Besides, the overall help information can be shown as below:

```sh
$ asvclr
Program: asvclr (Accurate Structural Variant Caller for Long Reads)
Version: 1.1.1 (using htslib 1.12)

Usage:  asvclr  <command> [options] <REF_FILE> <BAM_FILE> [Region ...]

Description:
   REF_FILE      Reference file (required)
   BAM_FILE      Coordinate-sorted BAM file (required)
   Region        Reference regions to process: CHR|CHR:START-END.
                 If unspecified, all reference regions will be 
                 processed (optional)

Commands:
   detect        detect indel signatures in aligned reads
   assemble      assemble candidate regions
   call          call indels by alignments of local genome assemblies
   all           run the above commands in turn
```


## Usage

Alternatively, there are three steps to run ASVCLR: `detect`, `assemble` and `call`.

```sh
$ asvclr detect -o out_dir hg38.fa hg38_ngmlr_sorted.bam
$ asvclr assemble -o out_dir hg38.fa hg38_ngmlr_sorted.bam
$ asvclr call -o out_dir hg38.fa hg38_ngmlr_sorted.bam
```

The reference and an sorted BAM file will be the input of ASVCLR, and the variants stored in the BED file format and translocations in BEDPE file format will be generated as the output.


### `Detect` Step
 
Structural variant regions will be detected according to variant signatures. These regions includes insertions, deletions, duplications, inversions and translocations.

```sh
$ asvclr detect -o out_dir hg38.fa hg38_ngmlr_sorted.bam
```

And the help information are shown below:

```sh
$ asvclr detect
Program: asvclr (Accurate Structural Variant Caller for Long Reads)
Version: 1.1.1 (using htslib 1.12)

Usage: asvclr detect [options] <REF_FILE> <BAM_FILE> [Region ...]

Description:
   REF_FILE      Reference file (required)
   BAM_FILE      Coordinate-sorted BAM file (required)
   Region        Reference regions to process: CHR|CHR:START-END.
                 If unspecified, all reference regions will be 
                 processed (optional)

Options: 
   -b INT        block size [1000000]
   -s INT        Slide window size [500]
   -m INT        minimal SV size to report [2]
   -M INT        maximal SV size to report [50000].
                 Variants with size smaller than threshold will be ignored
   -n INT        minimal number of reads supporting a SV [7]
   -e INT        minimal clipping end size [200]. Clipping events
                 with size smaller than threshold will be ignored
   -o DIR        output directory [output]
   -p STR        prefix of output result files [null]
   -t INT        number of concurrent work [0]. 0 for the maximal number
                 of threads in machine
   --include-decoy
                 include decoy items in result
   --sample STR  Sample name ["sample"]
   -v,--version  show version information
   -h,--help     show this help message and exit
```

### `Assemble` Step

Perform local assembly for the detected variant regions using Canu, and extract the corresponding local reference.

```sh
$ asvclr assemble -o out_dir hg38.fa hg38_ngmlr_sorted.bam
```

And the help information are shown below:

```sh
$ asvclr assemble
Program: asvclr (Accurate Structural Variant Caller for Long Reads)
Version: 1.1.1 (using htslib 1.12)

Usage: asvclr assemble [options] <REF_FILE> <BAM_FILE>

Description:
   REF_FILE      Reference file (required)
   BAM_FILE      Coordinate-sorted BAM file (required)

Options: 
   -e INT        minimal clipping end size [200]. Clipping events
                 with size smaller than threshold will be ignored
   -x FLOAT      expected sampling coverage for local assemble [30], 
                 0 for no coverage sampling
   -o DIR        output directory [output]
   -p STR        prefix of output result files [null]
   -t INT        number of concurrent work [0]. 0 for the maximal number
                 of threads in machine
   --threads-per-assem-work INT
                 Limited number of threads for each assemble work [0]:
                 0 for unlimited, and positive INT for the limited
                 number of threads for each assemble work
   --assem-chunk-size INT
                 maximal reference chunk size to collect reads data to perform
                 local assemble [10000]. Reads of variants with reference
                 distance < INT will be collected to perform local assemble
   --keep-assemble-reads
                 Keep temporary reads from being deleted during local assemble.
                 This may take some additional disk space
   --monitor_proc_names STR
                 Process names to be monitored during Canu assemble and BLAT alignment.
                 These processes may have ultra-high CPU running time under some certain
                 circumstances and should be terminated in advance if they become
                 computation intensive works. Note that the process names should be
                 comma-delimited and without blanks: ["overlapInCore,falconsense,blat"]
   --max_proc_running_minutes INT
                 Monitored processes will be terminated if their CPU running time exceed
                 INT minutes: [120]
   --technology STR
                 Sequencing technology [pacbio]:
                   pacbio     : the PacBio CLR sequencing technology;
                   nanopore   : the Nanopore sequencing technology;
                   pacbio-hifi: the PacBio CCS sequencing technology.
   --include-decoy
                 include decoy items in result
   --sample STR  Sample name ["sample"]
   -v,--version  show version information
   -h,--help     show this help message and exit
```

Note that:
 (1) the `assemble` step can be re-run from last stop to avoid unnecessary recomputation, and the `-x` option can be used to sampling high local coverage to a relative lower coverage to accelerate assemble process if the expected sampling coverage option `-x` is specified as a positive value.
 (2) As each Canu assemble work uses multiple threads by default, the `-T` option can be used to specify the limited number of threads for each assemble work.

### `Call` Step

Align the assembly result (contigs) to its local reference using BLAT to generate the sim4 formated alignments, and call each type variations using the BLAT alignments.

```sh
$ asvclr call -o out_dir hg38.fa hg38_ngmlr_sorted.bam
```

And the help information are shown below:

```sh
$ asvclr call
Program: asvclr (Accurate Structural Variant Caller for Long Reads)
Version: 1.1.1 (using htslib 1.12)

Usage: asvclr call [options] <REF_FILE> <BAM_FILE>

Description:
   REF_FILE      Reference file (required)
   BAM_FILE      Coordinate-sorted BAM file (required)

Options: 
   -m INT        minimal SV size to report [2]
   -M INT        maximal SV size to report [50000]
                 Variants with size smaller than threshold will be ignored
   -e INT        minimal clipping end size [200]. Clipping events
                 with size smaller than threshold will be ignored
   -o DIR        output directory [output]
   -p STR        prefix of output result files [null]
   -t INT        number of concurrent work [0]. 0 for the maximal number
                 of threads in machine
   --monitor_proc_names STR
                 Process names to be monitored during Canu assemble and BLAT alignment.
                 These processes may have ultra-high CPU running time under some certain
                 circumstances and should be terminated in advance if they become
                 computation intensive works. Note that the process names should be
                 comma-delimited and without blanks: ["overlapInCore,falconsense,blat"]
   --max_proc_running_minutes INT
                 Monitored processes will be terminated if their CPU running time exceed
                 INT minutes: [120]
   --include-decoy
                 include decoy items in result
   --sample STR  Sample name ["sample"]
   -v,--version  show version information
   -h,--help     show this help message and exit
```


## Output Result Description

Variant detection results are reported in two kinds of file format: VCF format and BED/BEDPE format. And in ASVCLR, variants of these two formats are equivalent.

### (1) VCF file format

Variants can be reported in VCF file format, and specifically, translocations are recorded by their breakend information which typically can be denoted as `BND` type, and a translocation variant may have 8 breakends at most, i.e. 8 `BND` items. In ASVCLR, the VCF file format can be described as below:

```sh
##fileformat=VCFv4.2
##fileDate=20210509
##source=ASVCLR 0.10.0
##reference=spombe_972h.fa
##PG="asvclr all -o output spombe_972h.fa ngmlr_spombe_sorted.bam"
##contig=<ID=chr1,length=5579133>
##contig=<ID=chr2,length=4539804>
##contig=<ID=chr3,length=2452883>
##contig=<ID=chrM,length=19431>
##phasing=None
##INFO=<ID=END,Number=1,Type=Integer,Description="End position of the structural variant described in this record">
##INFO=<ID=SVTYPE,Number=1,Type=String,Description="Type of structural variant">
##INFO=<ID=SVLEN,Number=1,Type=Integer,Description="Difference in length between REF and ALT alleles">
##INFO=<ID=DUPNUM,Number=1,Type=Integer,Description="Copy number of DUP">
##INFO=<ID=MATEID,Number=.,Type=String,Description="ID of mate breakends">
##INFO=<ID=MATEDIST,Number=1,Type=Integer,Description="Distance to the mate breakend for mates on the same contig">
##INFO=<ID=BLATINNER,Number=1,Type=Flag,Description="variants called from inner parts of BLAT align segment">
##FORMAT=<ID=GT,Number=1,Type=String,Description="Genotype">
##FORMAT=<ID=AD,Number=R,Type=Integer,Description="Read depth per allele">
##FORMAT=<ID=DP,Number=1,Type=Integer,Description="Read depth at this position for this sample">
#CHROM	POS	ID	REF	ALT	QUAL	FILTER	INFO	FORMAT	sample
chr1	3433	.	CA	CAA	.	PASS	SVTYPE=INS;SVLEN=1;END=3434;BLATINNER	GT	./.
chr1	2810113	.	TATTAAAA	TA	.	PASS	SVTYPE=DEL;SVLEN=-6;END=2810120;BLATINNER	GT	./.
chr1	1000001	BND.chr1:1000001-chr1:2000002	A	A[chr1:2000002[	.	PASS	SVTYPE=BND;MATEID=BND.chr1:2000002-chr1:1000001;MATEDIST=1000001	GT:AD:DP	./.:49,0:49
chr1	2000002	BND.chr1:2000002-chr1:1000001	A	]chr1:1000001]A	.	PASS	SVTYPE=BND;MATEID=BND.chr1:1000001-chr1:2000002;MATEDIST=1000001	GT:AD:DP	./.:49,0:49
chr1	1000002	BND.chr1:1000002-chr1:2000001	T	]chr1:2000001]T	.	PASS	SVTYPE=BND;MATEID=BND.chr1:2000001-chr1:1000002;MATEDIST=999999	GT:AD:DP	./.:40,1:41
chr1	2000001	BND.chr1:2000001-chr1:1000002	A	A[chr1:1000002[	.	PASS	SVTYPE=BND;MATEID=BND.chr1:1000002-chr1:2000001;MATEDIST=999999	GT:AD:DP	./.:40,1:41
chr1	1004999	BND.chr1:1004999-chr1:2005000	C	C[chr1:2005000[	.	PASS	SVTYPE=BND;MATEID=BND.chr1:2005000-chr1:1004999;MATEDIST=1000001	GT:AD:DP	./.:39,1:40
chr1	2005000	BND.chr1:2005000-chr1:1004999	T	]chr1:1004999]T	.	PASS	SVTYPE=BND;MATEID=BND.chr1:1004999-chr1:2005000;MATEDIST=1000001	GT:AD:DP	./.:39,1:40
chr1	1005000	BND.chr1:1005000-chr1:2004999	C	]chr1:2004999]C	.	PASS	SVTYPE=BND;MATEID=BND.chr1:2004999-chr1:1005000;MATEDIST=999999	GT:AD:DP	./.:44,1:45
chr1	2004999	BND.chr1:2004999-chr1:1005000	G	G[chr1:1005000[	.	PASS	SVTYPE=BND;MATEID=BND.chr1:1005000-chr1:2004999;MATEDIST=999999	GT:AD:DP	./.:44,1:45
```
The last 8 variant items corresponds to a translocation which locates between chr1:1000001-2005000 and chr2:2000002-2005000


### (2) BED/BEDPE file format

Variants also can be reported in BED/BEDPE file format in the output directory. Specifically, BED/BEDPE format consists of two different record types: BED format and BEDPE format. Insertions, deletions, inversions and duplications are stored in the BED file format, and translocations are saved in the BEDPE file format. The final variant results are saved in the BED/BEDPE file format and stored in the folder `4_results` in the output directory which can be specified with `-o` option.

Insertions, deletions, inversions and duplications are recorded in BED file format which are encoded as 8 columns in ASVCLR, and these columns can be described as below:

```sh
#Chr	Start	End	SVType	SVLen	DupNum	Ref	Alt
```
`DupNum` is the number of copies for the tandem duplications, `Ref` is the reference sequence in variant regions, and the `Alt` is the sample sequence in the variant regions.

Translocations are recored in BEDPE file format, and the first 13 columns can be described as below:

```sh
#Chr1	Start1	End1	Chr2	Start2	End2	SVType	SVLen1	SVLen2	MateReg	Ref	Alt1	Ref2	Alt2
```

Note that: In ASVCLR, all variant types, including translocations, can be stored together in the same BED file, for example:

```sh
#Chr    Start   End     SVType  SVLen  DupNum   Ref     Alt
#Chr1   Start1  End1    Chr2    Start2  End2    SVType  SVLen1  SVLen2  Ref1    Alt1    Ref2    Alt2
chr1	3033733	3033733	INS	41	-	C	CTTGCCTCTTGGATTTCATTCCTTGGTTAGTTTCTCTCAAAA
chr1	1185621	1185630	DEL	-9	-	AGTCCTATTG	A
chr2	3270134	3270184	INV	0	-	TTCCTTAAGAAACATTGTTGTTTTTAAAGTGAATTGATTGTCGCGGTTTCT	AGAAACCGCGACAATCAATTCACTTTAAAAACAACAATGTTTCTTAAGGAA
chr1	3081820	3085608	INV	3	-	TTCTATGTCATTTTTAATT...	AAAAAGAGGCCTGGACATCAATTGA...
chr3	1079105	1079633	DUP	1055	2	AGTTAATTCATTAATACTAATACTATCG...	AGTTAATTCATTAATACTAATACTATCGAGGATT...
chr1	1000002	2005000	chr2	2000002	2005000	TRA	4999	4999	2+|1000000|49|49,2+|999999|40|41;3+|1005000|39|40,3+|1004999|44|45;0+|2000000|40|40,0+|1999999|49|49;1+|2005000|44|44,1+|2004999|39|39	TATGAATGCCGCAGCTGGAAACTC...	AAAACGAGTTTTAGTTCAGTAGG...	AAAACGAGTTTTAGTTCAGTAGG...	TATGAATGCCGCAGCTGGAAACTC...
chr1	3000000	3010001	chr1	3999897	4010001	TRA	10002	10105	2+|3000000|35|36,2+|2999999|37|43;3+|3010001|35|38,3+|3010000|42|42;0+|3999897|32|32,0+|3999896|36|38;1+|4010001|43|43,1+|4010000|39|40	-	-	-	-
chr2    1500001 -       chr2    2500001 -       TRA     -       -       2+|1500001|33|34,2+|1500000|48|49;-;0+|2500001|49|49,0+|2500000|34|36;- -       -       -       -
chr2    1550000 -       chr2    2550001 -       TRA     -       -       2+|1550001|41|42,2+|1550000|43|43;-;0+|2550001|44|44,0+|2550000|41|43;- -       -       -       -
```
The last four variant items are translocations: the first two items are translocations which locates between chr1:1000002-2005000 and chr2:2000002-2005000, and between chr1:3000000-3010001 and chr1:3999897-4010001, respectively. And other two items are translocation breakpoints whose locations are chr2:1500001 and chr2:2500001, and chr2:1550000 and chr2:2550001, respectively.

------------------

## Contact

If you have problems or some suggestions, please contact: zhuxiao_hit@yeah.net without hesitation. 

---- Enjoy !!! -----

