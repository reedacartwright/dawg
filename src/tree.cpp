// tree.cc - Copyright (c) 2004-2009 Reed A. Cartwright (all rights reserved)

#include "dawg.h"
#include "tree.h"
#include "rand.h"

#include <algorithm>

using namespace std;

//void printnode(Tree::Sequence::node::pointer p) {
//	if(p == NULL)
//		return;
//	cout << "(";
//	printnode(p->left);
//	cout << p->val.base() << ((p->color) ? "r" : "b") << (p->weight.length);
//	printnode(p->right);
//	cout << ")";
//}
//
//void printsections(const Tree::Node::Sections &x) {
//	for(Tree::Node::Sections::const_iterator cit = x.begin();
//		cit != x.end(); ++cit)
//	{
//		for(Tree::Sequence::const_iterator it = cit->begin();
//			it != cit->end(); ++ it ) {
//				cerr << it->val.base();
//			}
//	}
//
//}

////////////////////////////////////////////////////////////
//  class NewickNode
////////////////////////////////////////////////////////////

// Construct a NewickNode from the parser
NewickNode::NewickNode(NewickNode* p, const char *cs, double d) : m_dLen(d), m_pSub(p)
{
	if(cs)
		m_ssLabel = cs;
	else
		MakeName();
}

// Construct a name from the descendents of a node
void NewickNode::MakeName()
{
	vector<const std::string*> v;
	NewickNode* p = m_pSub.get();
	while(p != NULL)
	{
		v.push_back(&p->m_ssLabel);
		p = p->m_pSib.get();
	}
	std::sort(v.begin(), v.end());
	m_ssLabel = "(";
	m_ssLabel += *v.front();
	for(vector<const std::string*>::iterator it = v.begin()+1; it != v.end(); ++it)
	{
		m_ssLabel += ",";
		m_ssLabel += **it;
	}
	m_ssLabel += ")";
}

////////////////////////////////////////////////////////////
//  class Sequence
////////////////////////////////////////////////////////////

//Sequence::const_iterator Sequence::SeqPos(size_type uPos) const
//{
//	const_iterator it = begin();
//	// Skip deletions
//	while(it->IsDeleted()) {++it;}
//	while(uPos--) {
//		++it;
//		// Skip deletions
//		while(it->IsDeleted()) {++it;}
//	}
//	return it;
//}

//Sequence::iterator Sequence::SeqPos(size_type uPos)
//{
//	iterator it = begin();
//	// Skip deletions
//	while(it->IsDeleted()) {++it;}
//	while(uPos--) {
//		++it;
//		// Skip deletions
//		while(it->IsDeleted()) {++it;}
//	}
//	return it;
//}

//// Insert itBegin to itEnd at itPos

//Sequence::size_type Sequence::Insertion(iterator itPos, const_iterator itBegin, const_iterator itEnd)
//{
//	if(itPos > end() || itPos < begin())
//		return 0;

//	size_type uRet = (size_type)(itEnd-itBegin);
//	insert(itPos, itBegin, itEnd);
//	m_uLength += uRet;
//	return uRet;
//}

//// Delete uSize nucleotides at itBegin
//Sequence::size_type Sequence::Deletion(iterator itBegin, size_type uSize)
//{
//	size_type uRet = 0;
//	for(;uRet < uSize && itBegin != end(); ++itBegin)
//	{
//		// Skip Gaps
//		if(itBegin->IsDeleted())
//			continue;
//		// Mark as Deleted-Root or Deleted-Insertion
//		itBegin->SetType(Nucleotide::TypeDel);
//		 ++uRet;
//	}
//	m_uLength -= uRet;
//	return uRet;
//}

//void Sequence::Append(const Sequence &seq)
//{
//		insert(end(), seq.begin(), seq.end());
//		m_uLength += seq.m_uLength;
//}

//void Sequence::ToString(std::string &ss) const
//{
//	ss.clear();
//	for(const_iterator cit = begin(); cit != end(); ++cit)
//		ss.push_back(cit->ToChar());
//}

//bool Nucleotide::FromChar(char ch)
//{
//	switch(ch&0xDF)
//	{
//	case 'A':
//		m_ucNuc = NumAdenine;
//		return true;
//	case 'C':
//		m_ucNuc = NumCytosine;
//		return true;
//	case 'G':
//		m_ucNuc = NumGuanine;
//		return true;
//	case 'T':
//		m_ucNuc = NumThymine;
//		return true;
//	}
//	return false;
//}
//char Nucleotide::ToChar() const
//{
//	static const char csNuc[]	= "ACGT";
//	static const char csType[]	= "-=+ ";

//	return IsExtant() ? csNuc[GetBase()] : csType[GetBase()];
//}

////////////////////////////////////////////////////////////
//  class Tree
////////////////////////////////////////////////////////////

bool Tree::ProcessTree(NewickNode* pNode)
{
	// Construct the ur-root node if it doesn't exist
	m_map["_R()()T"];
	// increase number of sections
	++m_nSec;
	// process the newick tree beginning at its root
	return ProcessNewickNode(pNode, "_R()()T");
}

bool Tree::ProcessNewickNode(NewickNode* pNode, const string &ssAnc)
{
	// Process all sibs of the Newick Node
	if(pNode->m_pSib.get() &&
		!ProcessNewickNode(pNode->m_pSib.get(), ssAnc))
		return false;

	// Get a reference to this node and set up parameters
	Node& node = m_map[pNode->m_ssLabel];
	if(!node.m_vAncestors.empty()) {
		if(pNode->m_dLen != node.m_dBranchLen)
			return DawgError("In recombinant tree Node \"%s\" has incompatable branch lengths to its parents.",
				node.m_ssName.c_str());
		node.m_vAncestors.resize(m_nSec, ssAnc);
		return true;
	}
	node.m_dBranchLen = pNode->m_dLen;
	node.m_vAncestors.push_back(ssAnc);

	// add node to tips if it is a tip and hasn't been accessed yet
	if(node.m_ssName.empty() && !pNode->m_pSub.get())
		m_vTips.push_back(pNode->m_ssLabel);

	// Give the node its name if it doesn't have one yet
	if(node.m_ssName.empty())
		node.m_ssName = pNode->m_ssLabel;

	// Process children
	if(pNode->m_pSub.get() &&
		!ProcessNewickNode(pNode->m_pSub.get(), node.m_ssName))
		return false;
	return true;
}

// Evolve the sequences in the tree
void Tree::Evolve()
{
	// Reset Sequences
	for(Node::Map::iterator it=m_map.begin(); it!=m_map.end();++it)
	{
		it->second.m_vSeq.clear();
		it->second.m_bTouched = false;
	}
	branchColor = 0;
	// Setup Root
	Node& rNode = m_map["_R()()T"];
	rNode.m_ssName = "_R()()T";
	rNode.m_bTouched = true;

	rNode.m_vSeq = m_vDNASeq;

	// Process Template
	unsigned int u=0;
	for(Sequence::iterator sit = rNode.m_vSeq.begin(); sit != rNode.m_vSeq.end(); ++sit) {
		if(sit->rate_scalar() < 0.0)
			sit->rate_scalar(static_cast<residue::rate_type>(RandomRate()));
		if(sit->is_deleted()) {
			sit->base(RandomBase());
			sit->mark_deleted(false);
		}
	}

	// Evolve each tip
	for(vector<string>::const_iterator cit = m_vTips.begin(); cit != m_vTips.end(); ++cit)
		Evolve(m_map[*cit]);
}

// Evolve node rNode
void Tree::Evolve(Node &rNode)
{
	// do nothing if it has already been touched
	if(rNode.m_bTouched)
		return;
	rNode.m_bTouched = true;

	// check to see if this is a recombination event
	if(rNode.m_vAncestors.size() > 1) {
		/*todo*/
		return;
	} else {
		// Touch Parent
		string &ssA = rNode.m_vAncestors.front();
		Node &aNode = m_map[ssA];
		Evolve(aNode);
		// Copy parent's sequence
		rNode.m_vSeq = aNode.m_vSeq;
	}
	Evolve(rNode, m_dTreeScale*rNode.m_dBranchLen);
}

// Evolve rNode a specific time
void Tree::Evolve(Node &rNode, double dTime)
{
	dTime = fabs(dTime);
	if(dTime < DBL_EPSILON)
		return; // Nothing to evolve

	//advance branch color
	branchColor += dawg::residue::branch_inc;

	//clear buffer
	m_vSeqBuffer.clear();
	const Sequence &seq = rNode.m_vSeq;
	double dT = 1.0/dTime;
	//insertion and deletion rates in float space
	dawg::residue::rate_type dIns = static_cast<dawg::residue::rate_type>(m_dLambdaIns);
	dawg::residue::rate_type dDel = static_cast<dawg::residue::rate_type>(m_dLambdaDel);
	dawg::residue::rate_type dIndel = dIns+dDel;
	Sequence::const_iterator cit = seq.begin();
	double d = rand_exp(dT);
	for(;;) {
		// process deletion here, which will allow for overlap.

		// draw exponention based on the branch length
		Sequence::const_iterator dit = cit;
		for(;dit != seq.end() && dit->rate_scalar()+dIndel <= d; ++dit) {
			if(!dit->is_deleted())
				d -= dIndel+dit->rate_scalar();
		}
		// copy unmodified sites into buffer.
		m_vSeqBuffer.insert(m_vSeqBuffer.end(), cit, dit);
		if(dit == seq.end())
			break;
		if(d < dDel) {
			d = rand_exp(dT);
			//cit =
			continue;
		} else {
			d -= dDel;
		}
		if(d < dit->rate_scalar()) {
			double w = dit->rate_scalar();
			residue rez = *cit;
			do {
				rez.base(m_dTransCum[rez.base()](rand_real()));
				// how much space is left in the substitution section
				w = w - d;
				d = rand_exp(dT);
			} while(d < w);
			d -= w;
			// push modified base
			m_vSeqBuffer.push_back(rez);
		} else {
			d -= dit->rate_scalar();
			m_vSeqBuffer.push_back(*cit);
		}
		if(d < dIns) {
			// Get Size of Insertion
			Sequence::size_type ul = m_pInsertionModel->RandSize();
			// Convert rate time to branch time
			// dT1 is the amount of time left in an insertion
			double dT1 = dT-dT*d/dIns;
			d = rand_exp(dT1);
			// Find site of next event.
			Sequence::size_type x = static_cast<size_type>(floor(d/dIns));
			if(x > ul)
				x = ul;
			while(x--) {

			}

			d = rand_exp(dT);
		} else {
			d -= dIns;
		}
		cit = dit;
		++cit;
	}
	rNode.m_vSeq.swap(m_vSeqBuffer);
}

Tree::Sequence::const_iterator Tree::EvolveIndels(
	// Beginning of the sequence
	Sequence::const_iterator itBegin,
	// End of the sequence
	Sequence::const_iterator itEnd,
	// Maximum time of the branch; origination time of the indel
	double dT, double dR,
	// Is the first one a deletion
	bool bDel, Sequence::size_type uLen) {
	double d, f, i;
	dawg::residue::rate_type dIns = static_cast<dawg::residue::rate_type>(m_dLambdaIns);
	dawg::residue::rate_type dDel = static_cast<dawg::residue::rate_type>(m_dLambdaDel);
	dawg::residue::rate_type dIndel = dIns+dDel;
	for(;;) {
		if(bDel) {
			d = rand_exp(dR);
			f = modf(d/dIndel, &i);
			Sequence::size_type x = static_cast<Sequence::size_type>(i);
			if(x < uLen) {
			}

		} else {
			d = rand_exp(dT-dR);
			f = modf(d/dIndel, &i);
			Sequence::size_type x = static_cast<Sequence::size_type>(i);
			if(x < uLen) {
				// Save tail for further processing
				m_sIndelData.push(IndelData(bDel, dR, uLen-x));
				// Add head to buffer
				while(x--)
					m_vSeqBuffer.push_back(RandomNucleotide());
				// Is next event a deletion of Insertion?
				if(f < dDel) {
					bDel = true;
					dR = (dT-dR)*f/dDel-dT;
					uLen = m_pDeletionModel->RandSize();
				} else {
					bDel = false;
					dR = (dT-dR)*(f-dDel)/dIns;
					uLen = m_pInsertionModel->RandSize();
				}
				continue;
			}
			while(uLen--)
				m_vSeqBuffer.push_back(RandomNucleotide());
		}
		if(m_sIndelData.empty())
			break;
		bDel = m_sIndelData.top().del;
		dR = m_sIndelData.top().orig;
		uLen = m_sIndelData.top().len;
		m_sIndelData.pop();
	}
	return itBegin;
}


// Setup Evolutionary parameters
bool Tree::SetupEvolution(double pFreqs[], double pSubs[],
		const IndelModel::Params& rIns,
		const IndelModel::Params& rDel,
		double dGamma,
		double dIota,
		double dTreeScale,
		int uKeepFlank)
{
	// Verifiy Parameters
	if(pFreqs[0] < 0.0 || pFreqs[1] < 0.0 || pFreqs[2] < 0.0 || pFreqs[3] < 0.0)
		return DawgError("Nucleotide frequences need to be positive.");
	pFreqs[3] = 1.0-pFreqs[0]-pFreqs[1]-pFreqs[2];
	if( pFreqs[3] < 0.0 )
		return DawgError("Nucleotide frequencies need to sum to 1.0.");
	if(pSubs[0] < 0.0 || pSubs[1] < 0.0 || pSubs[2] < 0.0
		|| pSubs[3] < 0.0 || pSubs[4] < 0.0 || pSubs[5] < 0.0)
		return DawgError("Substitution rates need to be positive.");

	if(rIns.dLambda < 0.0)
		return DawgError("Lambda (Ins) must not be negative.");
	if(rDel.dLambda < 0.0)
		return DawgError("Lambda (Del) must not be negative.");
	if(dGamma < 0.0)
		return DawgError("Invalid Gamma, \"%f\".  Gamma must be positive.", dGamma);
	if(0.0 > dIota || dIota > 1.0)
			return DawgError("Invalid Iota, \"%f\".  Iota must be a probability.", dIota);
	if(dTreeScale <= 0.0)
		return DawgError("Invalid TreeScale, \"%f\". TreeScale must be positive.", dTreeScale);

	// Setup Rate Parameters
	m_dGamma = dGamma;
	m_dIota = dIota;

	// Setup TreeScale
	m_dTreeScale = dTreeScale;

	// Setup GapLimit
	m_uKeepFlank = uKeepFlank;

	// Setup Cumulative Frequencies
	m_dFreqs[0] = pFreqs[0];
	m_dFreqs[1] = pFreqs[1];
	m_dFreqs[2] = pFreqs[2];
	m_dFreqs[3] = pFreqs[3];
	m_dFreqsCum = bitree<double>(&m_dFreqs[0], &m_dFreqs[4]);

	// Setup Symetric Matrix
	Matrix44 matQ(Matrix44::s_Zero);
	matQ(0,1) = matQ(1,0) = pSubs[0]; //A-C
	matQ(0,2) = matQ(2,0) = pSubs[1]; //A-G
	matQ(0,3) = matQ(3,0) = pSubs[2]; //A-T
	matQ(1,2) = matQ(2,1) = pSubs[3]; //C-G
	matQ(1,3) = matQ(3,1) = pSubs[4]; //C-T
	matQ(2,3) = matQ(3,2) = pSubs[5]; //G-T

	// Store Rate Matrix
	m_matR = matQ;

	// Create GTR Genetating Matrix
	Vector4 vecF(pFreqs);
	matQ.Scale(matQ, vecF);

	// Scale such that the total rate of substitution is equal to one
	double dX = 1.0/(m_dIota-1.0);
	matQ(0,0) = -(matQ(0,1)+matQ(0,2)+matQ(0,3));
	matQ(1,1) = -(matQ(1,0)+matQ(1,2)+matQ(1,3));
	matQ(2,2) = -(matQ(2,0)+matQ(2,1)+matQ(2,3));
	matQ(3,3) = -(matQ(3,0)+matQ(3,1)+matQ(3,2));
	matQ.Scale(matQ, dX/((vecF[0]*matQ(0,0)+vecF[1]*matQ(1,1)+
		vecF[2]*matQ(2,2)+vecF[3]*matQ(3,3))));

	// Store Scaled Q Matrix
	m_matQ = matQ;

	// Construct a Uniformized Transition Prob Matrix
	m_matTrans = m_matQ;
	double dMax = std::max(
		std::max(-m_matTrans(0,0), -m_matTrans(1,1)),
		std::max(-m_matTrans(2,2), -m_matTrans(3,3))
	);

	m_matTrans(0,0) = dMax+m_matTrans(0,0);
	m_matTrans(1,1) = dMax+m_matTrans(1,1);
	m_matTrans(2,2) = dMax+m_matTrans(2,2);
	m_matTrans(3,3) = dMax+m_matTrans(3,3);
	m_matTrans.Scale(m_matTrans, 1.0/dMax);

	m_dTransCum[0] = bitree<double>(&m_matTrans[0][0], &m_matTrans[0][4]);
	m_dTransCum[1] = bitree<double>(&m_matTrans[1][0], &m_matTrans[1][4]);
	m_dTransCum[2] = bitree<double>(&m_matTrans[2][0], &m_matTrans[2][4]);
	m_dTransCum[3] = bitree<double>(&m_matTrans[3][0], &m_matTrans[3][4]);

	// Correct TreeScale for Uniformization:
	m_dTreeScale *= dMax;

	// Make Q a symetric matrix again
	Vector4 vecD, vecE;  //D*E=I
	for(Matrix44::Pos i=0;i<4;++i)
	{
		vecD[i] = sqrt(vecF[i]);
		vecE[i] = 1.0/vecD[i];
	}
	matQ.Scale(matQ, vecE);
	matQ.Scale(vecD, matQ);

	//Find EigenSystem using Jacobian Transformations
	int nRet = EigenSystem(matQ, m_vecL, m_matV);
	if(nRet == -1)
		return DawgError("Eigensystem failed to converge.");
	m_matU = m_matV;
	m_matV.Scale(vecE, m_matV);
	m_matU.Transpose();
	m_matU.Scale(m_matU, vecD);

	// Setup Indel formation model
	// Insertion Rate
	m_dLambdaIns = rIns.dLambda;
	// Length Model
	if(m_dLambdaIns < DBL_EPSILON)
		m_pInsertionModel.release();
	else if(rIns.ssModel == "NB")
	{
		try {m_pInsertionModel.reset(new NegBnModel(rIns.vdModel));}
			catch(...) {return DawgError("Insertion model parameters not specified correctly.");}
	}
	else if(rIns.ssModel == "PL")
	{
		try {m_pInsertionModel.reset(new PowerModel(rIns.vdModel));}
			catch(...) {return DawgError("Insertion model parameters not specified correctly.");}
	}
	else if(rIns.ssModel == "US")
	{
		try {m_pInsertionModel.reset(new UserModel(rIns.vdModel));}
			catch(...) {return DawgError("Insertion model parameters not specified correctly.");}
	}
	else
		return DawgError("Unknown insertion model: \"%s\".", rDel.ssModel.c_str());

	// Deletion Rate
	m_dLambdaDel = rDel.dLambda;
	// Length model
	if(m_dLambdaDel < DBL_EPSILON)
		m_pDeletionModel.release();
	else if(rDel.ssModel == "NB")
	{
		try {m_pDeletionModel.reset(new NegBnModel(rDel.vdModel));}
			catch(...) {return DawgError("Deletion model parameters not specified correctly.");}
	}
	else if(rDel.ssModel == "PL")
	{
		try {m_pDeletionModel.reset(new PowerModel(rDel.vdModel));}
			catch(...) {return DawgError("Deletion model parameters not specified correctly.");}
	}
	else if(rDel.ssModel == "US")
	{
		try {m_pDeletionModel.reset(new UserModel(rDel.vdModel));}
			catch(...) {return DawgError("Deletion model parameters not specified correctly.");}
	}
	else
		return DawgError("Unknown deletion model: \"%s\".", rDel.ssModel.c_str());

	// Linear Functions for the Gillespie algorithm
	m_funcRateIns.m = m_dLambdaIns;
	m_funcRateIns.b = m_dLambdaIns;
	m_funcRateSum.m = m_dLambdaDel+m_dLambdaIns;
	m_funcRateSum.b = m_dLambdaIns+m_dLambdaDel*( (m_pDeletionModel.get()) ? m_pDeletionModel->MeanSize()-1.0 : 0.0);
	return true;
}

// Setup Root Template
bool Tree::SetupRoot(const std::vector<std::string> &vSeqs, const std::vector<unsigned int> &vLens,
					   const std::vector<std::vector<double> > &vRates)
{
	// Clear Template
	m_vDNASeq.clear();

	// Check to see if sequence is specified
	if(vSeqs.size()) {
		// Read sequence of each section
		for(unsigned int u = 0; u < vSeqs.size(); ++u) {
			const string & ss = vSeqs[u];
			for(string::const_iterator it = ss.begin(); it != ss.end(); ++it)
				m_vDNASeq.insert(m_vDNASeq.end(), make_seq(*it));
		}
	} else {
		// Create random sequences
		residue res(0, -1.0f, 0, true);
		for(unsigned int u = 0; u < vLens.size(); ++u)
			m_vDNASeq.insert(m_vDNASeq.end(), vLens[u], res);
	}
	// Check to see if rates are specified
	if(vRates.size()) {
		// Read rates of each section
		double dTemp = 0.0;
		Sequence::iterator it = m_vDNASeq.begin();
		for(unsigned int u=0; u < vRates.size(); ++u) {
			for(unsigned int v=0; v < vRates[u].size(); ++v) {
				it->rate_scalar(static_cast<residue::rate_type>(vRates[u][v]));
				dTemp += vRates[u][v];
				++it;
			}
		}
		// Scale the Expected Rate to 1.0
		for(; it != m_vDNASeq.end();++it) {
			residue::rate_type s = it->rate_scalar();
			it->rate_scalar(static_cast<residue::rate_type>(s/dTemp));
		}
	}
	return true;
}

Tree::Nucleotide::data_type Tree::RandomBase() const {
	return m_dFreqsCum(rand_real());
}

double Tree::RandomRate() const
{
	if(m_dIota > DBL_EPSILON && rand_bool(m_dIota))
		return 0.0;  // Site Invariant
	else if(m_dGamma > DBL_EPSILON)
		return rand_gamma1(m_dGamma); // Gamma with mean 1.0 and var of m_dGamma
	else
		return 1.0;
}

void Tree::Align(Alignment &aln, unsigned int uFlags)
{
	// construct a table of flattened sequences
	m_vAlnTable.clear();
	// if we remove this call to flatten, we can get even faster
	for(Node::Map::const_iterator cit = m_map.begin(); cit != m_map.end(); ++cit) {
		// Skip any sequence that begin with one of the two special characters
		if(cit->second.m_ssName[0] == '(' || cit->second.m_ssName[0] == '_')
			continue;
		m_vAlnTable.push_back(AlignData(cit->second.m_ssName));
		m_vAlnTable.back().seq = &cit->second.m_vSeq;
		m_vAlnTable.back().it = m_vAlnTable.back().seq->begin();
	}
	// Alignment rules:
	// Insertion & Deleted Insertion  : w/ ins, deleted ins, or gap
	// Deletion & Original Nucleotide : w/ del, original nucl

	// States: Quit (0), Root(1), Ins(2), InsDel(4), Del (8)
	unsigned int uState = 1;
	unsigned int uBranch = 0;
	unsigned int uBranchN = 0;
	// Go through each column, adding gaps where neccessary
	while(uState != 0) {
		uState = 0; // Set to quit
		uBranch = 0; // Set to lowest branch
		// Find column state(s)
		for(vector<AlignData>::iterator sit = m_vAlnTable.begin(); sit != m_vAlnTable.end(); ++sit) {
			if(sit->it == sit->seq->end())
				continue; // Sequence is done
			uBranchN = sit->it->branch();
			if(uBranchN > uBranch) {
				uBranch = uBranchN;
				uState = (sit->it->is_deleted() ? 2 : 1);
			} else if(uBranchN == uBranch) {
				uState |= (sit->it->is_deleted() ? 2 : 1);
			}
		}
		if(uState == 0) // Stop Aligning
			break;
		bool rmEmpty = !(uFlags & FlagOutKeepEmpty);
		for(vector<AlignData>::iterator sit = m_vAlnTable.begin(); sit != m_vAlnTable.end(); ++sit) {
			if(sit->it == sit->seq->end()) {
				if(!(uState == 2 && rmEmpty))
					sit->seqAln.push_back(Nucleotide(2, 1.0f, 0, true));
				continue;
			} else if(uState == 2 && rmEmpty) {
				if(sit->it->branch() != uBranch)
					continue;
			} else if(sit->it->branch() != uBranch) {
				sit->seqAln.push_back(Nucleotide(2, 1.0f, 0, true));
				continue;
			} else if(!sit->it->is_deleted())
				sit->seqAln.push_back(*sit->it);
			else if(sit->it->branch() == 0)
				sit->seqAln.push_back(Nucleotide(0, 1.0f, 0, true));
			else
				sit->seqAln.push_back(Nucleotide(1, 1.0f, 0, true));
			++(sit->it);
		}
	}

	// Add aligned sequences to alingment set
	for(vector<AlignData>::iterator sit = m_vAlnTable.begin(); sit != m_vAlnTable.end(); ++sit) {
		transform(sit->seqAln.begin(), sit->seqAln.end(),
			std::back_inserter(aln[sit->ssName]), AlignData::Printer(make_seq));
	}
}

