_Genetic_Code   = 0;

#include "simpleBootstrap.bf";

SetDialogPrompt ("Please specify a nucleotide or amino-acid data file:");

DataSet ds = ReadDataFile (PROMPT_FOR_FILE);
DataSetFilter filteredData = CreateFilter (ds,1);

fprintf (stdout,"\n______________READ THE FOLLOWING DATA______________\n",ds);

SelectTemplateModel(filteredData);

_DO_TREE_REBALANCE_ = 1;


#include "queryTree.bf";

LikelihoodFunction lf = (filteredData,givenTree);

timer = Time(0);


Optimize (res,lf);
timer = Time(0)-timer;

fprintf (stdout, "\n______________RESULTS______________\nTime taken = ", timer, " seconds\nAIC Score = ", 
				  2(res[1][1]-res[1][0]),"\n",lf);

#include "categoryEcho.bf";
