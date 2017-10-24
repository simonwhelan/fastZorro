/*
	A fast clustering algorithm for PPs guided by a tree
*/


#include <algorithm>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>
#include <iostream>
#include <vector>
#include <numeric>
#include <regex>
#include <algorithm>
#include <iomanip>
#include "Tree.h"

class CCluster{
public:
	static CCluster *Instance();
	bool AddNames(std::vector <string> Names) { if(!_names.empty()) { return false; }_names = Names; return true; };
	bool AddTree(std::string Tree) { if(_tree.NoSeq() > 0) { return false; } _tree = CTree(Tree,_names); MakePairs(); return true; }

	// Access for the pairs needed for calculation
	vector <vector <int> > PairsToCalculate();							// Gets the list of pairs to calculate

	// The output information
	// clusterMethods: [0] : mean
	std::vector <std::vector <int> > OutputClusters(vector <double> PPs, double threshold, int clusterMethod = 0);		// The result of the clustering

private:
	bool _ready = false;
	int NoSeq() { return _names.size(); }
	static CCluster * _cluster;
	std::vector <string> _names;
	int _approxNumber = 10;																		// The number of pairwise comparisons to make for each split
	CTree _tree;
	// Values linked to the clustering
	void MakePairs();																			// Calculates the pairs needed for clustering
	std::vector <SSplit> _splits;																// The tree splits (not including trivial splits)
	std::vector <std::vector<std::vector<int> > > _pairs;										// The set of pairs examined to assess each split
	vector <vector <int> > GetPairs(int splitNum);												// Get the pairs for a specific split number
	bool TestSplit(int split2Test, double threshold, vector <double> &PPs, int testMethod = 0);	// Function that uses PPs to test a specific split
	vector <vector <int> > AddSplit(int split2Add, vector <vector <int> > curSplit);			// Adds the _split(split2Add) to the current set of splits

};