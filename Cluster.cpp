/*
	A fast clustering algorithm for PPs guided by a tree
*/


#include "Cluster.h"
#include "Tree.h"

using namespace::std;
#include <unordered_set>

// Some initialisation
CCluster *CCluster::_cluster = NULL;
CCluster * CCluster::Instance() {
	if(!_cluster) {
		_cluster = new CCluster();
	}
	return _cluster;
}

// MakePairs()
// ---
// Computes the distance matrix from the tree and uses it to select a set of pairs for testing during divvying
// Gets the splits from the tree
void CCluster::MakePairs() {
	assert(_tree.NoSeq() == NoSeq());
	// Splits
	assert(_splits.empty());
	// All the splits
	for(int i = 0; i < _tree.NoBra() ; i++) {
		_splits.push_back(_tree.GetSplit( i ));
	}
	// Distance matrix
	vector <double> distances = _tree.GetTreePW();
	vector <int> big, small;
	vector <tuple <int , int , double> > distSet;			// The set of pairwise comparisons (int i, int j) and their distance (double)
	vector <vector <int> > pairs2add;
	for(SSplit &split : _splits) {
		// Get the full list of pairwise comparisons in the splits
		if(split.Left.size() > split.Right.size()) {
			big = split.Left; small = split.Right;
		} else {
			big = split.Right; small = split.Left;
		}
		// Create the list of pairwise comparisons sorted by distance. x always small; y alway big
		for(int &x : small) {
			for(int &y : big) {
				distSet.push_back(make_tuple(x,y,distances[(y*NoSeq()) + x]));
			}
		}
		sort(begin(distSet), end(distSet), [](auto const &t1, auto const &t2) {
			return get<2>(t1) < get<2>(t2);
		});
		// Obtain samples. The rules are:
		// 1. Unique sequences from the small data set are the priority, each with a different partner
		vector <int> small_count(NoSeq(),0),big_count(NoSeq(),0);
		int small_max = ceil( ( (double) _approxNumber / (double) small.size() ) +1 );
		int big_max = ceil( ( (double) _approxNumber / (double) big.size() ) + 1);
		for(int i = 0; i < distSet.size(); i++) {
			if(small_count[get<0>(distSet[i])] >= small_max) { continue; }
			if(big_count[get<1>(distSet[i])] >= big_max) { continue; }
			pairs2add.push_back(vector<int>{get<0>(distSet[i]),get<1>(distSet[i])});
			small_count[get<0>(distSet[i])]++;
			big_count[get<1>(distSet[i])]++;
			if(pairs2add.size() >= _approxNumber) { break; }	// Finish when we have the full list
		}
		_pairs.push_back(pairs2add);
		// Clean up
		pairs2add.clear();
		distSet.clear();
		big.clear();
		small.clear();
	}
	_ready = true;
}

// Get the set of pairs for testing split numbered splitNum
vector <vector <int>> CCluster::GetPairs(int splitNum) {
	return _pairs[splitNum];
}

vector <vector <int> > CCluster::PairsToCalculate() {
	vector <vector <int> > retVec;
	// Get the list. Note only upper 1/2 of diagonal
	for(auto x : _pairs) {
		for(auto y : x) {
			if(y[0] > y[1]) {
				retVec.push_back( vector<int>{y[1],y[0]} );
			} else {
				retVec.push_back( vector<int>{y[0],y[1]} );
			}
		}
	}
	if(retVec.size() == 0) { cout << "\nCCluster::PairsToCalculate() :: Need to have pairs to calculate... Have you initialised properly? Is the Tree wrong?\n"; exit(-1); }
	// Sort it
	sort(retVec.begin(), retVec.end(), [](auto x, auto y) {
		return (x[1] < y[1] && x[0] < y[0]);
	});
	sort(retVec.begin(), retVec.end(), [](vector <int> x, vector <int> y) {
		if(x[0] < y[0]) { return true; }
		if(x[0] > y[0]) { return false; }
		if(x[1] < y[1]) { return true; }
		return false;
	});
	// Remove redundancy
	for(int i = 0 ; i < retVec.size() - 1; i++) {
		if(retVec[i+1][0] == retVec[i][0] && retVec[i+1][1] == retVec[i][1]) {
			retVec.erase(retVec.begin() + i + 1);
			i--;
		}
	}
	return retVec;
}

vector < vector <int> > CCluster::OutputClusters(vector <double> PPs, double threshold, int clusterMethod) {
	assert(_ready);
	vector <vector <int> > retSplits;
	vector <int> starter(NoSeq(),0);
	for(int i = 0; i < NoSeq(); i++) { starter[i] = i; }
	retSplits.push_back(starter);
	// Find what clusters to make. Can change this function
	for(int i = 0 ; i < _splits.size(); i++) {
		if(!TestSplit(i,threshold,PPs,clusterMethod)) {
			retSplits = AddSplit(i,retSplits);
		}
	}
	// Sort them so they're in a nice order; the structure of hte tree splits mean they might not be
	sort(retSplits.begin(), retSplits.end(),[](const vector<int>& a, const vector<int>& b) {
	  return a[0] < b[0];
	});
	return retSplits;
}

// Calculates a test statistic based on PPs and compares it to threshold. If greater it passes and returns true
bool CCluster::TestSplit(int split2Test, double threshold, vector <double> &PPs, int testMethod) {
	double testStat = 0;
	switch(testMethod) {
	case 0: 				// Compute the mean and compare to threshold
		for(vector <int>  &v : _pairs[split2Test]) {
			assert(v.size() == 2);
			testStat += PPs[(v[0] * NoSeq()) + v[1]];
		}
		if(testStat / (double) _pairs[split2Test].size() + DBL_EPSILON >= threshold) { return true; }
		break;
	default:
		cout << "\nUnknown testMethod passed to CCluster::TestSplit(...)\n"; exit(-1);
	};
	return false;
}

vector <vector <int> > CCluster::AddSplit(int split2Add, vector <vector <int> > curSplit) {
	vector <vector <int> > retSplit;
	// Find the split in curSplit that has elements from both Left and Right _split(split2Add) present
	for(vector <int> &split : curSplit) {
		unordered_set<int> split_set ( split.begin(),split.end() );
		bool inLeft = false, inRight = false;
		for(int &left : _splits[split2Add].Left) {
			if(split_set.find(left) != split_set.end()) { inLeft = true; break; }
		}
		for(int &right : _splits[split2Add].Right) {
			if(split_set.find(right) != split_set.end()) { inRight = true; break; }
		}
		if(!inLeft || !inRight) { retSplit.push_back(split); continue; }
		vector <int> newSplit;
		for(int &left : _splits[split2Add].Left) {
			if(split_set.find(left) != split_set.end()) { newSplit.push_back(left); }
		}
		assert(newSplit.size() > 0);
		retSplit.push_back(newSplit);		// Note: no sorting required because they're pre-sorted
		newSplit.clear();
		for(int &right : _splits[split2Add].Right) {
			if(split_set.find(right) != split_set.end()) { newSplit.push_back(right); }
		}
		assert(newSplit.size() > 0);
		retSplit.push_back(newSplit);		// Note: no sorting required because they're pre-sorted
	}
	return retSplit;
}


