/*

  Main class and structures for DC

  Copyright (C) 2011  Tao Ju

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public License
  (LGPL) as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef OCTREE_H
#define OCTREE_H

#include < stdio.h >
#include < stdlib.h >
#include < string.h >
#include < math.h >
#include < time.h >
#include "GeoCommon.h"
#include "eigen.h"
#include "HashMap.h"
#include "intersection.h"

// Clamp all minimizers to be inside the cell
//#define CLAMP

// If SOG vertices are relative to each cell
//#define SOG_RELATIVE

//#define EDGE_TEST_CONVEXITY
//#define EDGE_TEST_FLIPDIAGONAL
#define EDGE_TEST_NEW

//#define TESS_UNIFORM
//#define TESS_NONE

/* Tree nodes */
class OctreeNode
{
public:

	OctreeNode(){} ;

	// Get type
	virtual int getType() = 0 ;
};


class LeafNode : public OctreeNode
{
private:
	// Signs
	unsigned char signs;

public:
	// Depth
	char height ;


	// Minimizer
	float mp[3] ;
	int index ;

	// QEF
	float ata[6], atb[3], btb ;


	// Construction
	LeafNode( int ht, unsigned char sg, float coord[3] ) 
	{
		height = ht ;
		signs = sg ;
        int i;
		for (  i = 0 ; i < 6 ; i ++ )
		{
			ata[i] = 0 ;
		}
		for ( i = 0 ; i < 3 ; i ++ )
		{
			mp[i] = coord[i] ;
			atb[i] = 0 ;
		}

		btb = 0 ;
		index = -1 ;
	};

	// Construction by QEF
	LeafNode( int ht, unsigned char sg, int st[3], int len, int numint, float inters[12][3], float norms[12][3] ) 
	{
		height = ht ;
		signs = sg ;
		index = -1 ;
        int i;
		for (  i = 0 ; i < 6 ; i ++ )
		{
			ata[i] = 0 ;
		}
		for ( i = 0 ; i < 3 ; i ++ )
		{
			mp[i] = 0 ;
			atb[i] = 0 ;
		}
		btb = 0 ;

		float pt[3] ={0,0,0} ;
		if ( numint > 0 )
		{
			for ( i = 0 ; i < numint ; i ++ )
			{
				float* norm = norms[i] ;
				float* p = inters[i] ;
				
				// printf("Norm: %f, %f, %f Pts: %f, %f, %f\n", norm[0], norm[1], norm[2], p[0], p[1], p[2] ) ;
				
			
				// QEF
				ata[ 0 ] += (float) ( norm[ 0 ] * norm[ 0 ] );
				ata[ 1 ] += (float) ( norm[ 0 ] * norm[ 1 ] );
				ata[ 2 ] += (float) ( norm[ 0 ] * norm[ 2 ] );
				ata[ 3 ] += (float) ( norm[ 1 ] * norm[ 1 ] );
				ata[ 4 ] += (float) ( norm[ 1 ] * norm[ 2 ] );
				ata[ 5 ] += (float) ( norm[ 2 ] * norm[ 2 ] );
				
				double pn = p[0] * norm[0] + p[1] * norm[1] + p[2] * norm[2] ;
				
				atb[ 0 ] += (float) ( norm[ 0 ] * pn ) ;
				atb[ 1 ] += (float) ( norm[ 1 ] * pn ) ;
				atb[ 2 ] += (float) ( norm[ 2 ] * pn ) ;
				
				btb += (float) pn * (float) pn ;
				
				// Minimizer
				pt[0] += p[0] ;
				pt[1] += p[1] ;
				pt[2] += p[2] ;
			}
			
			pt[0] /= numint ;
			pt[1] /= numint ;
			pt[2] /= numint ;
			
			// Solve
			float mat[10] ;
			BoundingBoxf * box = new BoundingBoxf ;
			box->begin.x = (float) st[0] ;
			box->begin.y = (float) st[1] ;
			box->begin.z = (float) st[2] ;
			box->end.x = (float) st[0] + len ;
			box->end.y = (float) st[1] + len ;
			box->end.z = (float) st[2] + len ;
			
			float error = calcPoint( ata, atb, btb, pt, mp, box, mat ) ;
#ifdef CLAMP
			if ( mp[0] < st[0] || mp[1] < st[1] || mp[2] < st[2] ||
				mp[0] > st[0] + len || mp[1] > st[1] + len || mp[2] > st[2] + len )
			{
				mp[0] = pt[0] ;
				mp[1] = pt[1] ;
				mp[2] = pt[2] ;
			}
#endif
		}
		else
		{
			printf("Number of edge intersections in this leaf cell is zero!\n") ;
			mp[0] = st[0] + len / 2;
			mp[1] = st[1] + len / 2;
			mp[2] = st[2] + len / 2;
		}
		
	};


	// Get type
	int getType ( )
	{
		return 1 ;
	};

	// Get sign
	int getSign ( int index )
	{
		return (( signs >> index ) & 1 );		
	};

};

class PseudoLeafNode : public OctreeNode
{
private:
	// Signs
	unsigned char signs;

public:
	// Depth
	char height ;


	// Minimizer
	float mp[3] ;
	int index ;

	// QEF
	float ata[6], atb[3], btb ;

	// Children 
	OctreeNode * child[8] ;

	// Construction
	PseudoLeafNode ( int ht, unsigned char sg, float coord[3] ) 
	{
		height = ht ;

		signs = sg ;
        int i;
		for (  i = 0 ; i < 6 ; i ++ )
		{
			ata[i] = 0 ;
		}
		for ( i = 0 ; i < 3 ; i ++ )
		{
			mp[i] = coord[i] ;
			atb[i] = 0 ;
		}

		btb = 0 ;

		for ( i = 0 ; i < 8 ; i ++ )
		{
			child[i] = NULL ;
		}

		index = -1 ;
	};

	PseudoLeafNode ( int ht, unsigned char sg, float ata1[6], float atb1[3], float btb1, float mp1[3] ) 
	{
		height = ht ;
		signs = sg ;
        int i;
		for (  i = 0 ; i < 6 ; i ++ )
		{
			ata[i] = ata1[i] ;
		}
		for ( i = 0 ; i < 3 ; i ++ )
		{
			mp[i] = mp1[i] ;
			atb[i] = atb1[i] ;
		}

		btb = btb1 ;

		for ( i = 0 ; i < 8 ; i ++ )
		{
			child[i] = NULL ;
		}

		index = -1 ;
	};
	
	// Get type
	int getType ( )
	{
		return 2 ;
	};

	// Get sign
	int getSign ( int index )
	{
		return (( signs >> index ) & 1 );		
	};

};

class InternalNode : public OctreeNode
{
public:
	// Children 
	OctreeNode * child[8] ;

	// Construction
	InternalNode ( ) 
	{
		for ( int i = 0 ; i < 8 ; i ++ )
		{
			child[i] = NULL ;
		}
	};

	// Get type
	int getType ( )
	{
		return 0 ;
	};
};

/* Global variables */
const int edgevmap[12][2] = {{0,4},{1,5},{2,6},{3,7},{0,2},{1,3},{4,6},{5,7},{0,1},{2,3},{4,5},{6,7}};
const int edgemask[3] = { 5, 3, 6 } ;
const int vertMap[8][3] = {{0,0,0},{0,0,1},{0,1,0},{0,1,1},{1,0,0},{1,0,1},{1,1,0},{1,1,1}} ;
const int faceMap[6][4] = {{4, 8, 5, 9}, {6, 10, 7, 11},{0, 8, 1, 10},{2, 9, 3, 11},{0, 4, 2, 6},{1, 5, 3, 7}} ;
const int cellProcFaceMask[12][3] = {{0,4,0},{1,5,0},{2,6,0},{3,7,0},{0,2,1},{4,6,1},{1,3,1},{5,7,1},{0,1,2},{2,3,2},{4,5,2},{6,7,2}} ;
const int cellProcEdgeMask[6][5] = {{0,1,2,3,0},{4,5,6,7,0},{0,4,1,5,1},{2,6,3,7,1},{0,2,4,6,2},{1,3,5,7,2}} ;
const int faceProcFaceMask[3][4][3] = {
	{{4,0,0},{5,1,0},{6,2,0},{7,3,0}},
	{{2,0,1},{6,4,1},{3,1,1},{7,5,1}},
	{{1,0,2},{3,2,2},{5,4,2},{7,6,2}}
} ;
const int faceProcEdgeMask[3][4][6] = {
	{{1,4,0,5,1,1},{1,6,2,7,3,1},{0,4,6,0,2,2},{0,5,7,1,3,2}},
	{{0,2,3,0,1,0},{0,6,7,4,5,0},{1,2,0,6,4,2},{1,3,1,7,5,2}},
	{{1,1,0,3,2,0},{1,5,4,7,6,0},{0,1,5,0,4,1},{0,3,7,2,6,1}}
};
const int edgeProcEdgeMask[3][2][5] = {
	{{3,2,1,0,0},{7,6,5,4,0}},
	{{5,1,4,0,1},{7,3,6,2,1}},
	{{6,4,2,0,2},{7,5,3,1,2}},
};
const int processEdgeMask[3][4] = {{3,2,1,0},{7,5,6,4},{11,10,9,8}} ;

const int dirCell[3][4][3] = {
	{{0,-1,-1},{0,-1,0},{0,0,-1},{0,0,0}},
	{{-1,0,-1},{-1,0,0},{0,0,-1},{0,0,0}},
	{{-1,-1,0},{-1,0,0},{0,-1,0},{0,0,0}}
};
const int dirEdge[3][4] = {
	{3,2,1,0},
	{7,6,5,4},
	{11,10,9,8}
};


/**
 * Class for building and processing an octree
 */
class Octree
{
public:
	/* Public members */

	/// Root node
	OctreeNode* root ;

	/// Length of grid
	int dimen ;
	int maxDepth ;
	
	/// Has QEF?
	int hasQEF ;

	int faceVerts, edgeVerts, actualTris ;
	int founds, news ;

public:
	/**
	 * Constructor
	 */
	Octree ( char* fname ) ;

	/**
	 * Destructor
	 */
	~Octree ( ) ;

	/**
	 * DC Simplify
	 */
	void simplify ( float thresh ) ;

	/**
	 * Contour
	 */
	void genContour ( char* fname ) ;
	void genContourNoInter ( char* fname ) ;
	void genContourNoInter2 ( char* fname ) ;


private:
	/* Helper functions */

	/**
	 * Simplification
	 */
	OctreeNode* simplify( OctreeNode* node, int st[3], int len, float thresh ) ;
	
	/**
	 * Read SOG file
	 */
	void readSOG ( char* fname ) ;
	OctreeNode* readSOG ( FILE* fin, int st[3], int len, int ht, float origin[3], float range ) ;

	/**
	 * Read DCF file
	 */
	void readDCF ( char* fname ) ;
	OctreeNode* readDCF ( FILE* fin, int st[3], int len, int ht ) ;

	/**
	 * Contouring
	 */
	void generateVertexIndex( OctreeNode* node, int& offset, FILE* fout ) ;
	void cellProcContour ( OctreeNode* node, FILE* fout ) ;
	void faceProcContour ( OctreeNode* node[2], int dir, FILE* fout ) ;
	void edgeProcContour ( OctreeNode* node[4], int dir, FILE* fout ) ;
	void processEdgeWrite ( OctreeNode* node[4], int dir, FILE* fout ) ;
	void cellProcCount ( OctreeNode* node, int& nverts, int& nfaces ) ;
	void faceProcCount ( OctreeNode* node[2], int dir, int& nverts, int& nfaces ) ;
	void edgeProcCount ( OctreeNode* node[4], int dir, int& nverts, int& nfaces ) ;
	void processEdgeCount ( OctreeNode* node[4], int dir, int& nverts, int& nfaces ) ;

	void cellProcContourNoInter( OctreeNode* node, int st[3], int len, HashMap2* hash, TriangleList* list, int& numTris ) ;
	void faceProcContourNoInter( OctreeNode* node[2], int st[3], int len, int dir, HashMap2* hash, TriangleList* list, int& numTris ) ;
	void edgeProcContourNoInter( OctreeNode* node[4], int st[3], int len, int dir, HashMap2* hash, TriangleList* list, int& numTris ) ;
	void processEdgeNoInter( OctreeNode* node[4], int st[3], int len, int dir, HashMap2* hash, TriangleList* list, int& numTris ) ;

	void cellProcContourNoInter2( OctreeNode* node, int st[3], int len, HashMap* hash, IndexedTriangleList* tlist, int& numTris, VertexList* vlist, int& numVerts ) ;
	void faceProcContourNoInter2( OctreeNode* node[2], int st[3], int len, int dir, HashMap* hash, IndexedTriangleList* tlist, int& numTris, VertexList* vlist, int& numVerts ) ;
	void edgeProcContourNoInter2( OctreeNode* node[4], int st[3], int len, int dir, HashMap* hash, IndexedTriangleList* tlist, int& numTris, VertexList* vlist, int& numVerts ) ;
	void processEdgeNoInter2( OctreeNode* node[4], int st[3], int len, int dir, HashMap* hash, IndexedTriangleList* tlist, int& numTris, VertexList* vlist, int& numVerts ) ;
	
	/**
	 * Non-intersecting test and tesselation
	 */
	int testFace( int st[3], int len, int dir, float v1[3], float v2[3] ) ;
	int testEdge( int st[3], int len, int dir, OctreeNode* node[4], float v[4][3] ) ;
	void makeFaceVertex( int st[3], int len, int dir, OctreeNode* node1, OctreeNode* node2, float v[3] ) ;
	void makeEdgeVertex( int st[3], int len, int dir, OctreeNode* node[4], float mp[4][3], float v[3] ) ;
};


#endif