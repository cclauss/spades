#include "constructHashTable.hpp"
#include "common.hpp"
#include "graphConstruction.hpp"
#include "seq.hpp"
#include "graphVisualizer.hpp"
#define RIGHT 1
#define LEFT -1

int VertexCount;
char EdgeStr[1000000];
const int minIntersect = l - 3;


using namespace paired_assembler;

void constructGraph() {
	//	readsToPairs(parsed_reads, parsed_k_l_mers);
	//	pairsToSequences(parsed_k_l_mers, parsed_k_sequence);
	cerr << "Read edges" << endl;
	edgesMap edges = sequencesToMap(parsed_k_sequence, true);
	cerr << "go to graph" << endl;
	gvis::GraphScheme<int> g("Paired");
	cerr << "Start vertices" << endl;
	VertexCount = 0;
	createVertices(g, edges);
}

edgesMap sequencesToMap(string parsed_k_sequence, bool usePaired) {
	FILE *inFile = fopen(parsed_k_sequence.c_str(), "r");

	vector<VertexPrototype *> prototypes;
	edgesMap res;
	prototypes.reserve(maxSeqLength);
	int count = 0;
	while (1) {
		char s[maxSeqLength];
		int size, scanf_res;
		ll kmer;
		count++;
		if (!(count & ((1 << 16) - 1))) {
			cerr << count << "k-seq readed" << endl;
		}
		scanf_res = fscanf(inFile, "%lld %d", &kmer, &size);
		//		cerr<<scanf_res;
		if ((scanf_res) != 2) {

			if (scanf_res == -1) {
				cerr << "sequencesToMap finished reading";
				break;
			} else {
				cerr << "sequencesToMap error in reading headers";
				continue;
			}
		}
		prototypes.clear();
		forn(i, size) {
			scanf_res = fscanf(inFile, "%s", s);
			if (!scanf_res) {
				cerr << "sequencesToMap error in reading sequences";
			}
			//			cerr <<s;
			Sequence *seq;
			if (usePaired) {
				seq = new Sequence(s);
			} else
				seq = new Sequence(auxilary_lmer);
			VertexPrototype *v = new VertexPrototype(seq, 0);
			if (usePaired || !i)
				prototypes.pb(v);
		}
		res.insert(mp(kmer, prototypes));
	}
	return res;
}

void createVertices(gvis::GraphScheme<int> &g, edgesMap &edges) {
	int inD[MAX_VERT_NUMBER], outD[MAX_VERT_NUMBER];
	char Buffer[2000];
	int EdgeId = 0;
	verticesMap verts;
	cerr << "Start createVertices " << edges.size() << endl;
	forn(i,MAX_VERT_NUMBER) {
		inD[i] = 0;
		outD[i] = 0;
	}

	for (edgesMap::iterator iter = edges.begin(); iter != edges.end();) {
		int size = iter->second.size();
		ll kmer = iter->fi;
		forn(i, size) {
			if ((!(iter->se)[i]->used)) {
				int length = 1;
				sprintf(EdgeStr + 500000, "%s", decompress(kmer, k).c_str());
				(iter->se)[i]->used = 1;
				ll finishKmer = kmer & (~((ll) 3 << (2 * (k - 1))));
				Sequence *finishSeq = new Sequence(
						(iter->se)[i]->lower->Subseq(1,
								(iter->se)[i]->lower->size()));
				ll startKmer = kmer >> 2;
				Sequence *startSeq = new Sequence(
						(iter->se)[i]->lower->Subseq(0,
								(iter->se)[i]->lower->size() - 1));
				length += expandRight(edges, verts, finishKmer, finishSeq);
				int toVert = storeVertex(g, verts, finishKmer, finishSeq);

				int toleft = expandLeft(edges, verts, startKmer, startSeq);
				length += toleft;
				int fromVert = storeVertex(g, verts, startKmer, startSeq);
				cerr << EdgeId << ": (" << length << ") " << ((char*) (EdgeStr
						+ 500000 - toleft)) << endl;
				if ((length < 300) && (length > k - 1)) {
					EdgeStr[500000 - toleft + length] = 0;
					sprintf(Buffer, "\"%d: (%d) %s\"", EdgeId, length,
							EdgeStr + 500000 - toleft + k - 1);
				} else {
					sprintf(Buffer, "\"%d: (%d)\"", EdgeId, length);
				}
				EdgeId++;
				g.addEdge(fromVert, toVert, Buffer);
				outD[fromVert]++;
				inD[toVert]++;

			}
		}
		(iter->second).clear();
		edges.erase(iter++);
	}
	g.output();
	forn(i, VertexCount) {
		cerr << i << " +" << inD[i] << " -" << outD[i] << endl;
	}
}

int expandRight(edgesMap &edges, verticesMap &verts, ll &finishKmer,
		Sequence* &finishSeq) {
	int length = 0;
	verticesMap::iterator iter;
	while (1) {
		iter = verts.find(finishKmer);
		if (iter != verts.end()) {

			int size = iter->second.size();
			forn(i, size) {
				if (finishSeq->similar(*((iter->se)[i]->lower), minIntersect, 0)) {
					return length;
				}
			}
		}

		if (!checkUniqueWayLeft(edges, finishKmer, finishSeq)) {
			return length;
		}

		if (!goUniqueWayRight(edges, finishKmer, finishSeq)) {
			return length;
		} else {
			EdgeStr[500000 + k + length] = nucl(finishKmer & 3);
			length++;
			EdgeStr[500000 + k + length] = 0;
		}
	}
}

int expandLeft(edgesMap &edges, verticesMap &verts, ll &startKmer,
		Sequence* &startSeq) {
	int length = 0;

	while (1) {
		verticesMap::iterator iter = verts.find(startKmer);
		if (iter != verts.end()) {
			int size = iter->second.size();
			forn(i, size) {
				if (startSeq->similar(*((iter->se)[i]->lower), minIntersect, 0)) {

					return length;
				}
			}
		}
		if (!checkUniqueWayRight(edges, startKmer, startSeq)) {
			return length;
		}
		int go_res;
		if ( !(go_res = goUniqueWayLeft(edges, startKmer, startSeq))) {
			return length;
		} else {

			if (go_res != 2) {
				length++;
				EdgeStr[500000 - length] = nucl((startKmer >> ((k - 2) * 2)) & 3);
			}
			else {
				cerr << endl<<"kmer_expanding"<<endl;
			}
		}
	}
}

int goUniqueWayLeft(edgesMap &edges, ll &finishKmer, Sequence* &finishSeq) {
	int count = 0;
	Sequence *PossibleSequence;
	ll PossibleKmer = 0;
	int seqIndex = 0;
	edgesMap::iterator PossibleIter;
	for (int Nucl = 0; Nucl < 4; Nucl++) {
		ll tmpKmer = (ll) Nucl << (2 * (k - 1)) | finishKmer;
		edgesMap::iterator iter = edges.find(tmpKmer);
		if (iter != edges.end()) {
			int size = iter->second.size();
			forn(i, size) {
				if (finishSeq->similar(*((iter->se)[i]->lower), k, -1)) {
					count++;
					if (count > 1)
						return 0;
					PossibleKmer = tmpKmer;
					PossibleSequence = (iter->se)[i]->lower;
					seqIndex = i;
					PossibleIter = iter;

				}
			}
		}
	}
	bool sameK = false;
	if (count == 2) {
		edgesMap::iterator iter = edges.find(finishKmer);
		if (iter != edges.end()) {
			int size = iter->second.size();
			forn(i, size) {
				Sequence *Ps = (iter->se)[i]->lower;
				if (finishSeq->similar(*Ps, minIntersect, -1)) {
					if (*Ps == *finishSeq) {
						cerr << endl << "sameSeq";
						continue;
					}
					if ((iter->se)[i]->used)
						return 0;
					count++;
					if (count > 1)
						return 0;
					PossibleKmer = finishKmer;
					PossibleSequence = (iter->se)[i]->lower;
					seqIndex = i;
					PossibleIter = iter;

				}
			}
		}
//		assert(1 == 0);
		sameK = true;
		if (count == 1 && sameK) {
			assert("1 == 0");
			cerr << endl << "something" << endl;
		}
	}

	if (count == 1) {
		finishKmer = PossibleKmer >> 2;
		finishSeq = new Sequence(
				(PossibleIter->se)[seqIndex]->lower->Subseq(0,
						(PossibleIter->se)[seqIndex]->lower->size() - 1));//PossibleSequence;
		(PossibleIter->se)[seqIndex]->used = 1;
		return 1;
	}
	return 0;
}

int goUniqueWayRight(edgesMap &edges, ll &finishKmer, Sequence* &finishSeq) {
	int count = 0;
	Sequence *PossibleSequence;
	ll PossibleKmer = 0;
	int seqIndex = 0;
	edgesMap::iterator PossibleIter;
	for (int Nucl = 0; Nucl < 4; Nucl++) {
		ll tmpKmer = (ll) Nucl | (finishKmer << (2));
		edgesMap::iterator iter = edges.find(tmpKmer);
		if (iter != edges.end()) {
			int size = iter->second.size();
			forn(i, size) {
				Sequence *Ps = (iter->se)[i]->lower;
				if (finishSeq->similar(*Ps, minIntersect, 1)) {
					if ((iter->se)[i]->used)
						return 0;
					count++;
					if (count > 1)
						return 0;
					PossibleKmer = tmpKmer;
					PossibleSequence = Ps;
					seqIndex = i;
					PossibleIter = iter;

				}
			}
		}
	}
	bool sameK = false;
	if (count == 2) {
		edgesMap::iterator iter = edges.find(finishKmer);
		if (iter != edges.end()) {
			int size = iter->second.size();
			forn(i, size) {
				Sequence *Ps = (iter->se)[i]->lower;
				if (finishSeq->similar(*Ps, minIntersect, 1)) {
					if ((iter->se)[i]->used)
						return 0;
					count++;
					if (count > 1)
						return 0;
					PossibleKmer = finishKmer;
					PossibleSequence = (iter->se)[i]->lower;
					seqIndex = i;
					PossibleIter = iter;

				}
			}
		}
//		assert(1 == 0);
		sameK = true;
	}
	if (count == 1) {
		finishKmer = (PossibleKmer) & (~(((ll) 3) << (2 * (k - 1))));
		finishSeq = new Sequence(
				(PossibleIter->se)[seqIndex]->lower->Subseq(1,
						(PossibleIter->se)[seqIndex]->lower->size()));//PossibleSequence;
		(PossibleIter->se)[seqIndex]->used = 1;
		if (sameK)
			return 2;
		else
			return 1;
	} else {
		return 0;
	}
}

int countWays(vector<VertexPrototype *> &v, Sequence *finishSeq, int direction) {
	int count = 0;
	for (vector<VertexPrototype *>::iterator it = v.begin(); it != v.end(); ++it)
		if (finishSeq->similar(*((*it)->lower), minIntersect, direction)) {
			count++;
			if (count > 1) {
				return count;
			}
		}
	return count;
}

/*
 * Method adds nucleotide to the side of kMer defined by direction
 */
ll pushNucleotide(ll kMer, int length, int direction, int nucl) {
	if(direction == RIGHT) {
		return (ll) nucl | (kMer << (2));
	} else {
		return (ll) nucl << (2 * length) | kMer;
	}
}

int checkUniqueWay(edgesMap &edges, ll finishKmer, Sequence *finishSeq, int direction) {
	int count = 0;
	for (int Nucl = 0; Nucl < 4; Nucl++) {
		ll tmpKmer = pushNucleotide(finishKmer, k - 1, direction, Nucl);
		edgesMap::iterator iter = edges.find(tmpKmer);
		if (iter != edges.end()) {
			count += countWays(iter->second, finishSeq, direction);
			if (count > 1)
				return 0;
		}
	}
	return count == 1;
}

int checkUniqueWayLeft(edgesMap &edges, ll finishKmer, Sequence *finishSeq) {
	return checkUniqueWay(edges, finishKmer, finishSeq, LEFT);
}

int checkUniqueWayRight(edgesMap &edges, ll finishKmer, Sequence* finishSeq) {
	return checkUniqueWay(edges, finishKmer, finishSeq, RIGHT);
}

verticesMap::iterator addKmerToMap(verticesMap &verts, ll kmer) {
	verticesMap::iterator position = verts.find(kmer);
	if (position == verts.end()) {
		vector<VertexPrototype *> prototypes;
		return verts.insert(make_pair(kmer, prototypes)).first;
	} else {
		return position;
	}
}

/*First argument of result is id of the vertex. Second argument is true if new entry
 * was created and false otherwise
 *
 */
pair<int, bool> addVertexToMap(verticesMap &verts, ll newKmer, Sequence* newSeq) {
	verticesMap::iterator position = addKmerToMap(verts, newKmer);
	vector<VertexPrototype *> *sequences = &position->second;
	for (vector<VertexPrototype *>::iterator it = sequences->begin(); it
			!= sequences->end(); ++it) {
		Sequence *otherSequence = (*it)->lower;
		if (newSeq->similar(*otherSequence, minIntersect, 0)) {
			return make_pair((*it)->start, false);
		}
	}
	sequences->push_back(new VertexPrototype(newSeq, VertexCount));
	VertexCount++;
	return make_pair(VertexCount - 1, true);
}

int storeVertex(gvis::GraphScheme<int> &g, verticesMap &verts, ll newKmer,
		Sequence* newSeq) {
	pair<int, bool> addResult = addVertexToMap(verts, newKmer, newSeq);
	if (addResult.second) {
		g.addVertex(addResult.first, decompress(newKmer, k - 1));
	}
	return addResult.first;
}

void resetVertexCount() {
	VertexCount = 0;
}
