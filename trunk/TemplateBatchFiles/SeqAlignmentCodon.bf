/* START ALIGNMENT SETTINGS */

if (!_HY_NUC_CODON_HAVE_SCORE_MATRIX || Type(scoreMatrix)!="Matrix" || Rows (scoreMatrix) != 64)
{
	LoadFunctionLibrary ("chooseGeneticCode");
	LoadFunctionLibrary ("SeqAlignShared");
	alignOptions ["SEQ_ALIGN_CHARACTER_MAP"]="ACGT";
	LoadFunctionLibrary ("GrabBag");
	protScoreMatrix = alignOptions["SEQ_ALIGN_SCORE_MATRIX"];
	alignOptions = {};
	
	scoreMatrix  = {64,64};
	protLetters = "ARNDCQEGHILKMFPSTWYV";
	mapping 	= mapStrings (_hyphyAAOrdering, protLetters);	
	for (k = 0; k < 64; k += 1)
	{
		mappedK = mapping[_Genetic_Code[k]];
		if (mappedK >= 0)
		{
			for (k2 = k; k2 < 64; k2 += 1)
			{
				mappedK2 = mapping[_Genetic_Code[k2]];
				if (mappedK2 >= 0)
				{
					aScore = protScoreMatrix[mappedK][mappedK2];
				}
				else
				{
					aScore = -10000;
				}
				scoreMatrix[k][k2] = aScore;
				scoreMatrix[k2][k] = aScore;
			}
		}
		else
		{
			for (k2 = k; k2 < 64; k2 += 1)
			{
				scoreMatrix[k][k2] = -10000;
				scoreMatrix[k2][k] = -10000;
			}
		}
	}
	
	
	
	alignOptions ["SEQ_ALIGN_SCORE_MATRIX"] = 	scoreMatrix;
	alignOptions ["SEQ_ALIGN_GAP_OPEN"]		= 	40;
	alignOptions ["SEQ_ALIGN_AFFINE"]		=   1;
	alignOptions ["SEQ_ALIGN_GAP_OPEN2"]	= 	20;
	alignOptions ["SEQ_ALIGN_GAP_EXTEND2"]	= 	5;
	alignOptions ["SEQ_ALIGN_FRAMESHIFT"]	= 	10;
	alignOptions ["SEQ_ALIGN_CODON_ALIGN"]	= 	1;
	alignOptions ["SEQ_ALIGN_NO_TP"]		= 	1;
	alignOptions ["SEQ_ALIGN_CHARACTER_MAP"]=  "ACGT";
}


LoadFunctionLibrary ("SeqAlignmentNucShared");



