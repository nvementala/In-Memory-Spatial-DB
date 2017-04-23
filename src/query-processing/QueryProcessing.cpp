/*
 * QueryProcessing.cpp
 *
 *  Created on: Mar 16, 2017
 *      Author: tejal
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <map>
#include "../integration/query-processing.h"

using namespace std;

QueryProcessing::QueryProcessing() {
	catalopgptr = Catalog::Instance();
}

QueryResult QueryProcessing::processQuery (QueryTree qTree) {
	QueryResult queryResult;
	char rootType = qTree.getRootType();
	vector<Filter> leftFilter = qTree.getLeftBranch();
	vector<Filter> rightFilter = qTree.getRightBranch();
	PointCollection leftDataPoint = qTree.getLeftDataPoint();
	RectangleCollection leftDataRect = qTree.getLeftDataRect();
	PointCollection rightDataPoint = qTree.getRightDataPoint();
	RectangleCollection rightDataRect = qTree.getRightDataRect();

	// Initially set to Invalid, but it is set to appropriate result type when setting to result;
	queryResult.setResultType(INVALID);
	// left data is points
	if (!leftDataPoint.isEmpty() && leftDataRect.isEmpty()) {
		PointCollection leftResult = materializeBranch(rootType, leftFilter, leftDataPoint);
		// no right branch
		if (rootType == NO_JOIN || rootType == NO_JOIN_DI || rootType == NO_JOIN_SI) {
			queryResult.setResultType(TYPE_POINT);
			queryResult.setPointCollection(leftResult);
		}
		else {
			// right data is points
			if (!rightDataPoint.isEmpty() && rightDataRect.isEmpty()) {
				cout << "\nLeft & Right are Points\n";
				PointCollection rightResult = materializeBranch(rootType, rightFilter, rightDataPoint);
				if (rootType == KNN_JOIN || rootType == KNN_JOIN_DI || rootType == KNN_JOIN_SI) {
					cout << "\nPerforming KNN Join \n";
					cout << "\nRight Data Size : " << rightResult.getSize() << endl;
					cout << "\nLeft Data Size : " << leftResult.getSize() << endl;
					PointPointCollection joinResult =
							knnJoin(qTree.getRootParam(), leftResult, rightFilter, rightResult);
					queryResult.setResultType(TYPE_POINTPOINT);
					queryResult.setPointPointCollection(joinResult);
				}
				else if (rootType == RANGE_JOIN || rootType == RANGE_JOIN_DI || rootType == RANGE_JOIN_SI) {
					PointPointCollection joinResult = rangeJoin(leftResult, rightFilter, rightResult);
					queryResult.setResultType(TYPE_POINTPOINT);
					queryResult.setPointPointCollection(joinResult);
				}
				else if (rootType == DISTANCE_JOIN || rootType == DISTANCE_JOIN_DI || rootType == DISTANCE_JOIN_SI) {
					PointPointCollection joinResult =
							distanceJoin(qTree.getRootParam(), leftResult, rightFilter, rightResult);
					queryResult.setResultType(TYPE_POINTPOINT);
					queryResult.setPointPointCollection(joinResult);
				}
			}
			// right data is rectangles
			else if (rightDataPoint.isEmpty() && !rightDataRect.isEmpty()) {
				RectangleCollection rightResult = materializeBranch(rootType, rightFilter, rightDataRect);
				if (rootType == KNN_JOIN || rootType == KNN_JOIN_DI || rootType == KNN_JOIN_SI) {
					PointRectangleCollection joinResult =
							knnJoin(qTree.getRootParam(), leftResult, rightFilter, rightResult);
					queryResult.setResultType(TYPE_POINTRECTANGLE);
					queryResult.setPointRectangleCollection(joinResult);
				}
				else if (rootType == RANGE_JOIN || rootType == RANGE_JOIN_DI || rootType == RANGE_JOIN_SI) {
					PointRectangleCollection joinResult;
					if ((rootType == RANGE_JOIN_DI || rootType == RANGE_JOIN_SI) && rightFilter.empty()) {
						if (rootType == RANGE_JOIN_DI) {
							indexptr = catalopgptr->getDataIndexedCollection(rightDataRect.getDBName(), rightDataRect.getTableName());
						}
						else {
							indexptr = catalopgptr->getSpatialIndexedCollection(rightDataRect.getDBName(), rightDataRect.getTableName());
						}
						joinResult = rangeJoinWithIndex(leftResult, rightFilter, rightResult);
					}
					else {
						joinResult = rangeJoin(leftResult, rightFilter, rightResult);
					}
					queryResult.setResultType(TYPE_POINTRECTANGLE);
					queryResult.setPointRectangleCollection(joinResult);
				}
				else if (rootType == DISTANCE_JOIN || rootType == DISTANCE_JOIN_DI || rootType == DISTANCE_JOIN_SI) {
					PointRectangleCollection joinResult =
							distanceJoin(qTree.getRootParam(), leftResult, rightFilter, rightResult);
					queryResult.setResultType(TYPE_POINTRECTANGLE);
					queryResult.setPointRectangleCollection(joinResult);
				}
			}
		}
	}
	// left data is rectangles
	else if (leftDataPoint.isEmpty() && !leftDataRect.isEmpty()) {
		RectangleCollection leftResult = materializeBranch(rootType, leftFilter, leftDataRect);
		// no right branch
		if (rootType == NO_JOIN || rootType == NO_JOIN_DI || rootType == NO_JOIN_SI) {
			queryResult.setResultType(TYPE_RECTANGLE);
			queryResult.setRectangleCollection(leftResult);
		}
		else {
			// right data is points
			if (!rightDataPoint.isEmpty() && rightDataRect.isEmpty()) {
				PointCollection rightResult = materializeBranch(rootType, rightFilter, rightDataPoint);
				if (rootType == KNN_JOIN || rootType == KNN_JOIN_DI || rootType == KNN_JOIN_SI) {
					PointRectangleCollection joinResult =
							knnJoin(qTree.getRootParam(), leftResult, rightFilter, rightResult);
					queryResult.setResultType(TYPE_POINTRECTANGLE);
					queryResult.setPointRectangleCollection(joinResult);
				}
				else if (rootType == RANGE_JOIN || rootType == RANGE_JOIN_DI || rootType == RANGE_JOIN_SI) {
					PointRectangleCollection joinResult;
					if ((rootType == RANGE_JOIN_DI || rootType == RANGE_JOIN_SI) && rightFilter.empty()) {
						if (rootType == RANGE_JOIN_DI) {
							indexptr = catalopgptr->getDataIndexedCollection(leftDataRect.getDBName(), leftDataRect.getTableName());
						}
						else {
							indexptr = catalopgptr->getSpatialIndexedCollection(leftDataRect.getDBName(), leftDataRect.getTableName());
						}
						joinResult = rangeJoinWithIndex(rightResult, rightFilter, leftResult);
					}
					else {
						joinResult = rangeJoin(rightResult, rightFilter, leftResult);
					}
					queryResult.setResultType(TYPE_POINTRECTANGLE);
					queryResult.setPointRectangleCollection(joinResult);
				}
				else if (rootType == DISTANCE_JOIN || rootType == DISTANCE_JOIN_DI || rootType == DISTANCE_JOIN_SI) {
					PointRectangleCollection joinResult =
							distanceJoin(qTree.getRootParam(), rightResult, rightFilter, leftResult);
					queryResult.setResultType(TYPE_POINTRECTANGLE);
					queryResult.setPointRectangleCollection(joinResult);
				}
			}
			// right data is rectangles
			else if (rightDataPoint.isEmpty() && !rightDataRect.isEmpty()) {
				RectangleCollection rightResult = materializeBranch(rootType, rightFilter, rightDataRect);
				if (rootType == KNN_JOIN || rootType == KNN_JOIN_DI || rootType == KNN_JOIN_SI) {
					RectangleRectangleCollection joinResult =
							knnJoin(qTree.getRootParam(), leftResult, rightFilter, rightResult);
					queryResult.setResultType(TYPE_RECTANGLERECTANGLE);
					queryResult.setRectangleRectangleCollection(joinResult);
				}
				else if (rootType == RANGE_JOIN || rootType == RANGE_JOIN_DI || rootType == RANGE_JOIN_SI) {
					RectangleRectangleCollection joinResult = rangeJoin(leftResult, rightFilter, rightResult);
					if ((rootType == RANGE_JOIN_DI || rootType == RANGE_JOIN_SI) && rightFilter.empty()) {
						if (rootType == RANGE_JOIN_DI) {
							indexptr = catalopgptr->getDataIndexedCollection(rightDataRect.getDBName(), rightDataRect.getTableName());
						}
						else {
							indexptr = catalopgptr->getSpatialIndexedCollection(rightDataRect.getDBName(), rightDataRect.getTableName());
						}
						joinResult = rangeJoinWithIndex(leftResult, rightFilter, rightResult);
					}
					else {
						joinResult = rangeJoin(leftResult, rightFilter, rightResult);
					}
					queryResult.setResultType(TYPE_RECTANGLERECTANGLE);
					queryResult.setRectangleRectangleCollection(joinResult);
				}
				else if (rootType == DISTANCE_JOIN || rootType == DISTANCE_JOIN_DI || rootType == DISTANCE_JOIN_SI) {
					RectangleRectangleCollection joinResult =
							distanceJoin(qTree.getRootParam(), leftResult, rightFilter, rightResult);
					queryResult.setResultType(TYPE_RECTANGLERECTANGLE);
					queryResult.setRectangleRectangleCollection(joinResult);
				}
			}
		}

	}
	return queryResult;
}

PointCollection QueryProcessing::materializeBranch (char rootType, vector<Filter> filter, PointCollection data) {
	// initialize result
	vector<Point> pt_null;
	char dataOrientation = '0';//data.getCollectionStructure();
	PointCollection result("","",dataOrientation,pt_null);

	if (rootType == NO_JOIN_DI || rootType == RANGE_JOIN_DI || rootType == KNN_JOIN_DI || rootType == DISTANCE_JOIN_DI) {
		indexptr = catalopgptr->getDataIndexedCollection(data.getDBName(), data.getTableName());
		for (int i=0;i<filter.size();i++) {
			if (filter[i].filterType == OBJECTS_IN_RANGE) {
				Rectangle rec(filter[i].inputParams[0], filter[i].inputParams[1],
						filter[i].inputParams[2], filter[i].inputParams[3]);
				data = indexptr->searchPoint(rec);
				break;
			}
		}
	}
	else if (rootType == NO_JOIN_SI || rootType == RANGE_JOIN_SI || rootType == KNN_JOIN_SI || rootType == DISTANCE_JOIN_SI) {
		indexptr = catalopgptr->getSpatialIndexedCollection(data.getDBName(), data.getTableName());
		for (int i=0;i<filter.size();i++) {
			if (filter[i].filterType == OBJECTS_IN_RANGE) {
				Rectangle rec(filter[i].inputParams[0], filter[i].inputParams[1],
						filter[i].inputParams[2], filter[i].inputParams[3]);
				data = indexptr->searchPoint(rec);
				break;
			}
		}
	}

	vector<Point> points = data.getNext(data.getSize());
	for (int i=0;i<filter.size();i++) {
		if (filter[i].filterType == KNN) {
			if (filter[i].inputParams.size()==5) {
				Rectangle r(filter[i].inputParams[1], filter[i].inputParams[2],
													filter[i].inputParams[3], filter[i].inputParams[4]);
				points = getKnnPointsFromRectangle((int)filter[i].inputParams[0],r,points);
			}
			else {
				Point p(filter[i].inputParams[1], filter[i].inputParams[2]);
				points = getKnnPointsFromPoint((int)filter[i].inputParams[0],p,points);
			}
		}
	}
	int j = 0;
	while (j < data.getSize()) {
		bool passedAllOperators = true;
		for (int i=0;i<filter.size();i++) {
			if (filter[i].filterType != KNN ||
					((rootType == NO_JOIN_DI || rootType == RANGE_JOIN_DI || rootType == KNN_JOIN_DI || rootType == DISTANCE_JOIN_DI)
							&& filter[i].filterType != OBJECTS_IN_RANGE)) {
				passedAllOperators = passedAllOperators && opDict.applyOperator(filter[i],points[j]);
			}
		}
		if (passedAllOperators) {
			result.insert(points[j]);
		}
		j++;
	}
	return result;
}

RectangleCollection QueryProcessing::materializeBranch (char rootType, vector<Filter> filter, RectangleCollection data) {
	// initialize result
	RectangleCollection result;

	if (rootType == NO_JOIN_DI || rootType == RANGE_JOIN_DI || rootType == KNN_JOIN_DI || rootType == DISTANCE_JOIN_DI) {
		indexptr = catalopgptr->getDataIndexedCollection(data.getDBName(), data.getTableName());
		for (int i=0;i<filter.size();i++) {
			if (filter[i].filterType == OBJECTS_IN_RANGE) {
				Rectangle rec(filter[i].inputParams[0], filter[i].inputParams[1],
						filter[i].inputParams[2], filter[i].inputParams[3]);
				data = indexptr->searchRectangle(rec);
				break;
			}
		}
	}
	else if (rootType == NO_JOIN_SI || rootType == RANGE_JOIN_SI || rootType == KNN_JOIN_SI || rootType == DISTANCE_JOIN_SI) {
		indexptr = catalopgptr->getSpatialIndexedCollection(data.getDBName(), data.getTableName());
		for (int i=0;i<filter.size();i++) {
			if (filter[i].filterType == OBJECTS_IN_RANGE) {
				Rectangle rec(filter[i].inputParams[0], filter[i].inputParams[1],
						filter[i].inputParams[2], filter[i].inputParams[3]);
				data = indexptr->searchRectangle(rec);
				break;
			}
		}
	}

	vector<Rectangle> rects = data.getNext(data.getSize());
	for (int i=0;i<filter.size();i++) {
		if (filter[i].filterType == KNN) {
			if (filter[i].inputParams.size()==5) {
				Rectangle r(filter[i].inputParams[1], filter[i].inputParams[2],
													filter[i].inputParams[3], filter[i].inputParams[4]);
				rects = getKnnRectanglesFromRectangle((int)filter[i].inputParams[0],r,rects);
			}
			else {
				Point p(filter[i].inputParams[1], filter[i].inputParams[2]);
				rects = getKnnRectanglesFromPoint((int)filter[i].inputParams[0],p,rects);
			}
		}
	}
	int j = 0;
	while (j < data.getSize()) {
		bool passedAllOperators = true;
		for (int i=0;i<filter.size();i++) {
			if (filter[i].filterType != KNN ||
					((rootType == NO_JOIN_DI || rootType == RANGE_JOIN_DI || rootType == KNN_JOIN_DI || rootType == DISTANCE_JOIN_DI)
							&& filter[i].filterType != OBJECTS_IN_RANGE)) {
				passedAllOperators = passedAllOperators && opDict.applyOperator(filter[i],rects[j]);
			}
		}
		if (passedAllOperators) {
			result.insert(rects[j]);
		}
		j++;
	}
	return result;
}

vector<Point> QueryProcessing::getKnnPointsFromPoint (int k, Point inputPoint, vector<Point> inputPoints) {
	vector<Point> knnPoints;
	for (int i=0;i<inputPoints.size();i++) {
		if (knnPoints.size()<k) {
			knnPoints.push_back(inputPoints[i]);
		}
		else {
			int distance = PointOperations::getDistance(inputPoint, inputPoints[i]);
			for (int j=0; j<k; j++) {
				if (distance < PointOperations::getDistance(inputPoint, knnPoints[j])) {
					knnPoints.erase(knnPoints.begin() + j);
					knnPoints.push_back(inputPoints[i]);
					break;
				}
			}
		}
	}
	return knnPoints;
}

vector<Point> QueryProcessing::getKnnPointsFromRectangle (int k, Rectangle inputRect, vector<Point> inputPoints) {
	vector<Point> knnPoints;
	for (int i=0;i<inputPoints.size();i++) {
		if (knnPoints.size()<k) {
			knnPoints.push_back(inputPoints[i]);
		}
		else {
			int distance = PointOperations::getDistance(inputPoints[i], inputRect);
			for (int j=0; j<k; j++) {
				if (distance < PointOperations::getDistance(knnPoints[j], inputRect)) {
					knnPoints.erase(knnPoints.begin() + j);
					knnPoints.push_back(inputPoints[i]);
					break;
				}
			}
		}
	}
	return knnPoints;
}

vector<Rectangle> QueryProcessing::getKnnRectanglesFromPoint (int k, Point inputPoint, vector<Rectangle> inputRectangles) {
	vector<Rectangle> knnRectangles;
	for (int i=0;i<inputRectangles.size();i++) {
		if (knnRectangles.size()<k) {
			knnRectangles.push_back(inputRectangles[i]);
		}
		else {
			int distance = PointOperations::getDistance(inputPoint, inputRectangles[i]);
			for (int j=0; j<k; j++) {
				if (distance < PointOperations::getDistance(inputPoint, knnRectangles[j])) {
					knnRectangles.erase(knnRectangles.begin() + j);
					knnRectangles.push_back(inputRectangles[i]);
					break;
				}
			}
		}
	}
	return knnRectangles;
}

vector<Rectangle> QueryProcessing::getKnnRectanglesFromRectangle (int k, Rectangle inputRect, vector<Rectangle> inputRectangles) {
	vector<Rectangle> knnRectangles;
	for (int i=0;i<inputRectangles.size();i++) {
		if (knnRectangles.size()<k) {
			knnRectangles.push_back(inputRectangles[i]);
		}
		else {
			int distance = RectangleOperations::getDistance(inputRectangles[i], inputRect);
			for (int j=0; j<k; j++) {
				if (distance < RectangleOperations::getDistance(knnRectangles[j], inputRect)) {
					knnRectangles.erase(knnRectangles.begin() + j);
					knnRectangles.push_back(inputRectangles[i]);
					break;
				}
			}
		}
	}
	return knnRectangles;
}

bool alongX(const Point &p1, const Point &p2){
	return p1.getX() < p2.getX();
}

bool alongY(const Point &p1, const Point &p2){
	return p1.getY() < p2.getY();
}

PointPointCollection QueryProcessing::sweepBasedJoin (PointCollection leftData, PointCollection rightData, bool onX) {

	cout << " Debug : Using sweepBasedJoin \n";
	vector<PointPoint> joinResultVector;

	//bool sort_on_X = true;
	int idx = onX ? 0 : 1;

	vector<Point> leftPoints = leftData.getNext(leftData.getSize());
	//sort(leftPoints.begin(), leftPoints.end(), onX ? alongX : alongY);	// Sorted based on a dimension

	map<long int, vector<Point> > leftMap;										// Cogrouping the ranges of keys
	long int key = 0;
	for (Point pt : leftPoints){
		key = pt.getCoordinates()[idx] * 100000;
		leftMap[key].push_back(pt);
	}

	vector<Point> rightPoints = rightData.getNext(rightData.getSize());
	//sort(rightPoints.begin(), rightPoints.end(), onX ? alongX : alongY);  // Sorted based on a dimension

	map<long int, vector<Point> > rightMap;										// Cogrouping the ranges of keys
	key = 0;
	for (Point pt : rightPoints){
		key = pt.getCoordinates()[idx] * 100000;
		rightMap[key].push_back(pt);
	}

	map<long int, vector<Point> >::iterator leftItr = leftMap.begin();
	map<long int, vector<Point> >::iterator rightItr = rightMap.begin();
	int comparisions = 0;
	while( leftItr != leftMap.end() && rightItr != rightMap.end()){
		if (leftItr->first == rightItr->first){
			//Do Join
			vector<Point> leftSub = leftItr->second;
			vector<Point> rightSub = rightItr->second;
			for (Point pt_l : leftSub){
				for (Point pt_r : rightSub){
					comparisions++;
					if( PointOperations::isOverlapping(pt_l, pt_r) || PointOperations::isEqual(pt_l, pt_r)){
						PointPoint pp(pt_l.getCoordinates()[0], pt_l.getCoordinates()[1], pt_r.getCoordinates()[0], pt_r.getCoordinates()[1]);
						joinResultVector.insert(joinResultVector.end(), pp);
					}
				}
			}
			leftItr++; rightItr++;
		}
		else if (leftItr->first < rightItr->first){
			leftItr++;
		}
		else{
			rightItr++;
		}
	}

	cout << "Num of comparisions (sweep) : " << comparisions;
	PointPointCollection rangeJoinResult(POINTPOINT,DB_NAME,TYPE_POINTPOINT,joinResultVector);
	return rangeJoinResult;
}

PointPointCollection QueryProcessing::rangeJoin (PointCollection leftData, vector<Filter> filter,
		PointCollection rightData) {

	/*if (leftData.getCollectionStructure() == COLLECTION_STRUCT_SORTEDX && rightData.getCollectionStructure() == COLLECTION_STRUCT_SORTEDX){
		cout << "Found both data sorted on X (performing sweep join)";
		return sweepBasedJoin(leftData, rightData, true);
	}
	else if (leftData.getCollectionStructure() == COLLECTION_STRUCT_SORTEDY && rightData.getCollectionStructure() == COLLECTION_STRUCT_SORTEDY){
		cout << "Found both data sorted on Y (performing sweep join)";
		return sweepBasedJoin(leftData, rightData, false);
	}*/

	vector<PointPoint> joinResultVector;
	vector<Point> leftPoints = leftData.getNext(leftData.getSize());
	vector<Point> rightPoints = rightData.getNext(rightData.getSize());
	int comparisions = 0;
	for (int i=0;i<leftPoints.size();i++) {
		for (int j=0;j<rightPoints.size();j++) {
			comparisions++;
			if (PointOperations::isEqual(leftPoints[i], rightPoints[j])) {
				PointPoint pp(leftPoints[i].getCoordinates()[0],leftPoints[i].getCoordinates()[1],
						rightPoints[j].getCoordinates()[0],rightPoints[j].getCoordinates()[1]);
				joinResultVector.insert(joinResultVector.end(),pp);
			}
		}
	}
	cout << "Num of comparisions (nested): " << comparisions << endl;
	PointPointCollection rangeJoinResult(POINTPOINT,DB_NAME,TYPE_POINTPOINT,joinResultVector);
	return rangeJoinResult;
}

RectangleRectangleCollection QueryProcessing::rangeJoin (RectangleCollection leftData, vector<Filter> filter,
		RectangleCollection rightData) {

	vector<RectangleRectangle> joinResultVector;
	vector<Rectangle> leftRects = leftData.getNext(leftData.getSize());
	vector<Rectangle> rightRects = rightData.getNext(rightData.getSize());

	for (int i=0;i<leftRects.size();i++) {
		for (int j=0;j<rightRects.size();j++) {
			if (RectangleOperations::isOverlapping(leftRects[i],rightRects[j]) ||
					RectangleOperations::isIntersecting(leftRects[i],rightRects[j]) ||
					RectangleOperations::isWithin(leftRects[i],rightRects[j]) ||
					RectangleOperations::isWithin(rightRects[j],leftRects[i])) {
				RectangleRectangle rr(leftRects[i].getCoordinates()[0],leftRects[i].getCoordinates()[1],
						leftRects[i].getCoordinates()[2],leftRects[i].getCoordinates()[3],
						rightRects[j].getCoordinates()[0],rightRects[j].getCoordinates()[1],
						rightRects[j].getCoordinates()[2],rightRects[j].getCoordinates()[3]);
				joinResultVector.insert(joinResultVector.end(),rr);
			}
		}
	}

	RectangleRectangleCollection rangeJoinResult(RECTANGLERECTANGLE,DB_NAME,TYPE_RECTANGLERECTANGLE,joinResultVector);
	return rangeJoinResult;
}

PointRectangleCollection QueryProcessing::rangeJoin (PointCollection leftData, vector<Filter> filter,
		RectangleCollection rightData) {
	vector<PointRectangle> joinResultVector;
	vector<Point> leftPoints = leftData.getNext(leftData.getSize());
	vector<Rectangle> rightRects = rightData.getNext(rightData.getSize());

	for (int i=0;i<leftPoints.size();i++) {
		for (int j=0;j<rightRects.size();j++) {
			if (PointOperations::isOverlapping(leftPoints[i],rightRects[j]) ||
					PointOperations::isIntersecting(leftPoints[i],rightRects[j]) ||
					PointOperations::isWithin(leftPoints[i],rightRects[j])) {
				PointRectangle pr(leftPoints[i].getCoordinates()[0],leftPoints[i].getCoordinates()[1],
						rightRects[j].getCoordinates()[0],rightRects[j].getCoordinates()[1],
						rightRects[j].getCoordinates()[2],rightRects[j].getCoordinates()[3]);
				joinResultVector.insert(joinResultVector.end(),pr);
			}
		}
	}
	PointRectangleCollection rangeJoinResult(POINTRECTANGLE,DB_NAME,TYPE_POINTRECTANGLE,joinResultVector);
	return rangeJoinResult;
}

RectangleRectangleCollection QueryProcessing::rangeJoinWithIndex (RectangleCollection leftData, vector<Filter> filter,
		RectangleCollection rightData) {

	vector<RectangleRectangle> joinResultVector;
	vector<Rectangle> leftRects = leftData.getNext(leftData.getSize());

	for (int i=0;i<leftRects.size();i++) {
		rightData = indexptr->searchRectangle(leftRects[i]);
		vector<Rectangle> rightRects = rightData.getNext(rightData.getSize());
		for (int j=0;j<rightRects.size();j++) {
			RectangleRectangle rr(leftRects[i].getCoordinates()[0],leftRects[i].getCoordinates()[1],
					leftRects[i].getCoordinates()[2],leftRects[i].getCoordinates()[3],
					rightRects[j].getCoordinates()[0],rightRects[j].getCoordinates()[1],
					rightRects[j].getCoordinates()[2],rightRects[j].getCoordinates()[3]);
			joinResultVector.insert(joinResultVector.end(),rr);
		}
	}

	RectangleRectangleCollection rangeJoinResult(RECTANGLERECTANGLE,DB_NAME,TYPE_RECTANGLERECTANGLE,joinResultVector);
	return rangeJoinResult;
}

PointRectangleCollection QueryProcessing::rangeJoinWithIndex (PointCollection leftData, vector<Filter> filter,
		RectangleCollection rightData) {
	vector<PointRectangle> joinResultVector;
	vector<Rectangle> rightRects = rightData.getNext(rightData.getSize());
	for (int j=0;j<rightRects.size();j++) {
		leftData = indexptr->searchPoint(rightRects[j]);
		vector<Point> leftPoints = leftData.getNext(leftData.getSize());
		for (int i=0;i<leftPoints.size();i++) {
			PointRectangle pr(leftPoints[i].getCoordinates()[0],leftPoints[i].getCoordinates()[1],
					rightRects[j].getCoordinates()[0],rightRects[j].getCoordinates()[1],
					rightRects[j].getCoordinates()[2],rightRects[j].getCoordinates()[3]);
			joinResultVector.insert(joinResultVector.end(),pr);
		}
	}
	PointRectangleCollection rangeJoinResult(POINTRECTANGLE,DB_NAME,TYPE_POINTRECTANGLE,joinResultVector);
	return rangeJoinResult;
}

PointPointCollection QueryProcessing::knnJoin (float k, PointCollection leftData, vector<Filter> filter,
		PointCollection rightData) {
	vector<PointPoint> joinResultVector;
	vector<Point> leftPoints = leftData.getNext(leftData.getSize());
	vector<Point> rightPoints = rightData.getNext(rightData.getSize());
	cout << "\nInside KNN Join \n";
	cout << "\nRight Data Size : " << leftPoints.size();
	cout << "\nLeft Data Size : " << rightPoints.size();
	for (int i=0;i<leftPoints.size();i++) {
		vector<Point> rightKnnPoints = getKnnPointsFromPoint((int)k, leftPoints[i], rightPoints);
		for (int j=0;j<rightKnnPoints.size();j++) {
			PointPoint pp(leftPoints[i].getCoordinates()[0],leftPoints[i].getCoordinates()[1],
					rightKnnPoints[j].getCoordinates()[0],rightKnnPoints[j].getCoordinates()[1]);
			joinResultVector.insert(joinResultVector.end(),pp);
		}
	}
	cout << " ** " << joinResultVector.size();
	PointPointCollection joinResult(POINTPOINT,DB_NAME,TYPE_POINTPOINT,joinResultVector);
	return joinResult;
}

RectangleRectangleCollection QueryProcessing::knnJoin (float k, RectangleCollection leftData, vector<Filter> filter,
		RectangleCollection rightData) {
	vector<RectangleRectangle> joinResultVector;
	vector<Rectangle> leftRects = leftData.getNext(leftData.getSize());
	vector<Rectangle> rightRects = rightData.getNext(rightData.getSize());
	for (int i=0;i<leftRects.size();i++) {
		vector<Rectangle> rightKnnRects = getKnnRectanglesFromRectangle((int)k, leftRects[i], leftRects);
		for (int j=0;j<rightKnnRects.size();j++) {
			RectangleRectangle rr(leftRects[i].getCoordinates()[0],leftRects[i].getCoordinates()[1],
					leftRects[i].getCoordinates()[2],leftRects[i].getCoordinates()[3],
					rightKnnRects[j].getCoordinates()[0],rightKnnRects[j].getCoordinates()[1],
					rightKnnRects[j].getCoordinates()[2],rightKnnRects[j].getCoordinates()[3]);
			joinResultVector.insert(joinResultVector.end(),rr);
		}
	}
	RectangleRectangleCollection joinResult(RECTANGLERECTANGLE,DB_NAME,TYPE_RECTANGLERECTANGLE,joinResultVector);
	return joinResult;
}

PointRectangleCollection QueryProcessing::knnJoin (float k, PointCollection leftData, vector<Filter> filter,
		RectangleCollection rightData) {
	vector<PointRectangle> joinResultVector;
	vector<Point> leftPoints = leftData.getNext(leftData.getSize());
	vector<Rectangle> rightRects = rightData.getNext(rightData.getSize());
	for (int i=0;i<leftPoints.size();i++) {
		vector<Rectangle> rightKnnRects  = getKnnRectanglesFromPoint((int)k, leftPoints[i], rightRects);
		for (int j=0;j<rightKnnRects.size();j++) {
			PointRectangle pr(leftPoints[i].getCoordinates()[0],leftPoints[i].getCoordinates()[1],
					rightKnnRects[j].getCoordinates()[0],rightKnnRects[j].getCoordinates()[1],
					rightKnnRects[j].getCoordinates()[2],rightKnnRects[j].getCoordinates()[3]);
			joinResultVector.insert(joinResultVector.end(),pr);
		}
	}
	PointRectangleCollection joinResult(POINTRECTANGLE,DB_NAME,TYPE_POINTRECTANGLE,joinResultVector);
	return joinResult;
}

PointRectangleCollection QueryProcessing::knnJoin (float k, RectangleCollection leftData, vector<Filter> filter,
		PointCollection rightData) {
	vector<PointRectangle> joinResultVector;
	vector<Rectangle> leftRects = leftData.getNext(leftData.getSize());
	vector<Point> rightPoints = rightData.getNext(rightData.getSize());
	for (int i=0;i<leftRects.size();i++) {
		vector<Point> rightKnnPoints = getKnnPointsFromRectangle((int)k, leftRects[i], rightPoints);
		for (int j=0;j<rightKnnPoints.size();j++) {
			PointRectangle pr(rightKnnPoints[j].getCoordinates()[0],rightKnnPoints[j].getCoordinates()[1],
					leftRects[i].getCoordinates()[0],leftRects[i].getCoordinates()[1],
					leftRects[i].getCoordinates()[2],leftRects[i].getCoordinates()[3]);
			joinResultVector.insert(joinResultVector.end(),pr);
		}
	}
	PointRectangleCollection joinResult(POINTRECTANGLE,DB_NAME,TYPE_POINTRECTANGLE,joinResultVector);
	return joinResult;
}

PointPointCollection QueryProcessing::distanceJoin (float distThresh, PointCollection leftData,
		vector<Filter> filter, PointCollection rightData) {
	vector<PointPoint> joinResultVector;
	vector<Point> leftPoints = leftData.getNext(leftData.getSize());
	vector<Point> rightPoints = rightData.getNext(rightData.getSize());
	for (int i=0;i<leftPoints.size();i++) {
		for (int j=0;j<rightPoints.size();j++) {
			if (PointOperations::getDistance(leftPoints[i],rightPoints[j]) <= distThresh) {
				PointPoint pp(leftPoints[i].getCoordinates()[0],leftPoints[i].getCoordinates()[1],
						rightPoints[j].getCoordinates()[0],rightPoints[j].getCoordinates()[1]);
				joinResultVector.insert(joinResultVector.end(),pp);
			}
		}
	}
	PointPointCollection joinResult(POINTPOINT,DB_NAME,TYPE_POINTPOINT,joinResultVector);
	return joinResult;
}

RectangleRectangleCollection QueryProcessing::distanceJoin (float distThresh, RectangleCollection leftData,
		vector<Filter> filter, RectangleCollection rightData) {
	vector<RectangleRectangle> joinResultVector;
	vector<Rectangle> leftRects = leftData.getNext(leftData.getSize());
	vector<Rectangle> rightRects = rightData.getNext(rightData.getSize());
	for (int i=0;i<leftRects.size();i++) {
		for (int j=0;j<rightRects.size();j++) {
			if (RectangleOperations::getDistance(leftRects[i],rightRects[j]) <= distThresh) {
				RectangleRectangle rr(leftRects[i].getCoordinates()[0],leftRects[i].getCoordinates()[1],
						leftRects[i].getCoordinates()[2],leftRects[i].getCoordinates()[3],
						rightRects[j].getCoordinates()[0],rightRects[j].getCoordinates()[1],
						rightRects[j].getCoordinates()[2],rightRects[j].getCoordinates()[3]);
				joinResultVector.insert(joinResultVector.end(),rr);
			}
		}
	}
	RectangleRectangleCollection joinResult(RECTANGLERECTANGLE,DB_NAME,TYPE_RECTANGLERECTANGLE,joinResultVector);
	return joinResult;
}

PointRectangleCollection QueryProcessing::distanceJoin (float distThresh, PointCollection leftData,
		vector<Filter> filter, RectangleCollection rightData) {
	vector<PointRectangle> joinResultVector;
	vector<Point> leftPoints = leftData.getNext(leftData.getSize());
	vector<Rectangle> rightRects = rightData.getNext(rightData.getSize());
	for (int i=0;i<leftPoints.size();i++) {
		for (int j=0;j<rightRects.size();j++) {
			if (PointOperations::getDistance(leftPoints[i],rightRects[j]) <= distThresh) {
				PointRectangle pr(leftPoints[i].getCoordinates()[0],leftPoints[i].getCoordinates()[1],
						rightRects[j].getCoordinates()[0],rightRects[j].getCoordinates()[1],
						rightRects[j].getCoordinates()[2],rightRects[j].getCoordinates()[3]);
				joinResultVector.insert(joinResultVector.end(),pr);
			}
		}
	}
	PointRectangleCollection joinResult(POINTRECTANGLE,DB_NAME,TYPE_POINTRECTANGLE,joinResultVector);
	return joinResult;
}
