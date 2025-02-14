//
//  init.cpp
//  RelationExtraction
//
//  Created by Feng Jun on 06/12/2016.
//  Copyright © 2016 Feng Jun. All rights reserved.
//

#include "init.h"

string localPathString = "/Users/fengjun/Documents/Research/relation extraction/code/RelationExtraction";
//string serverPathString = "/home/fengjun/RelationExtraction/GithubCode1";

string serverPathString = "";
string pathString = "";
string version = "";
string outString = "";
int output_model = 0;

int num_threads = 10;
int trainTimes = 15;
int trainPreTimes = 50;
int sampleTimes = 3;
float InitialAlpha = 0.02;
float reduce = 0.98;
int tt,tt1;
int dimensionC = 230;//1000; // hidden size
int dimensionWPE = 5;//25;   // position embedding dim
int dimension;               // word embedding dim
int window = 3;
int limit = 30; // position feature limits
float marginPositive = 2.5;
float marginNegative = 0.5;
float margin = 2;
float Belt = 0.001;
// matrixW1: conv weights, flat vector, actual shape: [dimensionC, window, dimension]
// matrixB1: conv bias, flat vector, actual shape: [dimensionC]
// matrixRelation: output weights, shape [dimensionC, relationTotal]
// matrixRelationPr: output bias, shape [relationTotal]
// matrixRelationPrDao: shape [relationTotal]
// matrixRelationDao: shape [dimensionC, relationTotal]
float *matrixB1, *matrixRelation, *matrixW1, *matrixRelationDao, *matrixRelationPr, *matrixRelationPrDao;// array of floats
float *matrixB1_egs, *matrixRelation_egs, *matrixW1_egs, *matrixRelationPr_egs;
float *matrixB1_exs, *matrixRelation_exs, *matrixW1_exs, *matrixRelationPr_exs;
float *matrixW1Dao;// shape [dimensionC, window, dimension]
float *matrixB1Dao;// shape [dimensionC]

// wordVecDao: actual shape [dimension, wordTotal]
float *wordVecDao,*wordVec_egs,*wordVec_exs;

// matrixW1PositionE1: conv weights, flat vector, actual shape: [dimensionC, window, dimensionWPE]
// matrixW1PositionE2: conv weights, flat vector, actual shape: [dimensionC, window, dimensionWPE]
// positionVecE1: weights, flat vector, actual shape: [PositionTotalE1, dimensionWPE]
// positionVecE2: weights, flat vector, actual shape: [PositionTotalE2, dimensionWPE]
float *positionVecE1, *positionVecE2, *matrixW1PositionE1, *matrixW1PositionE2;
float *positionVecE1_egs, *positionVecE2_egs, *matrixW1PositionE1_egs, *matrixW1PositionE2_egs, *positionVecE1_exs, *positionVecE2_exs, *matrixW1PositionE1_exs, *matrixW1PositionE2_exs;
float *matrixW1PositionE1Dao; // shape [dimensionC, window, dimensionWPE]
float *matrixW1PositionE2Dao;// shape [dimensionC, window, dimensionWPE]
float *positionVecDaoE1;// shape [PositionTotalE1, dimensionWPE]
float *positionVecDaoE2;// shape [PositionTotalE2, dimensionWPE]

double mx = 0;
int batch = 16;
int npoch;
int len;
float rate = 1;
float eps = 0.0000000001;
FILE *logg;
FILE *prlog;

float *wordVec;   // normed word embedding, weights, actual shape [wordTotal, dimension]
int wordTotal, relationTotal; 
int PositionMinE1, PositionMaxE1, PositionTotalE1,PositionMinE2, PositionMaxE2, PositionTotalE2;
map<string,int> wordMapping; // map words to ids
vector<string> wordList;     // map ids to words
map<string,int> relationMapping; // map relations to ids

// train data
vector<int *> trainLists, trainPositionE1, trainPositionE2; // sentence, position feature of all train data, indexed by instance id
vector<int> trainLength;                      // sentence lengths of all train data, indexed by instance id
vector<int> headList, tailList, relationList; // text e1_id e2_id relation_id of all train data, indexed by instance id

// test data
vector<int *> testtrainLists, testPositionE1, testPositionE2;
vector<int> testtrainLength;
vector<int> testheadList, testtailList, testrelationList;

vector<std::string> nam; // relaiton set

vector<float *> sentenceVec;// shape [totalInstance, dimensionC]
vector<double>  lossVec;    // shape [totalInstance]
// featureList: state F(si) in paper, 1)sentence vec 2) avg chosen sentence set vec 3) head and tail entity in kb
vector<float> featureList;// shape dimension * 2 + dimensionC * 2 + 1
float* featureW;    // shape dimension * 2 + dimensionC * 2 + 1
float* bestFeatureW;// shape dimension * 2 + dimensionC * 2 + 1
float* featureWDao; // shape dimension * 2 + dimensionC * 2 + 1
int featureLen = 0;
float freezeRatio = 0.001;

// updateMatrixRelationPr: shape [relationTotal]
// updateMatrixRelation: shape [dimensionC, relationTotal]
// updateMatrixW1: shape [dimensionC, window, dimension]
// updateMatrixB1: shape [dimensionC]
float *updateMatrixB1, *updateMatrixRelation, *updateMatrixW1, *updateMatrixRelationPr;
// updatePositionVecE1: shape [PositionTotalE1, dimensionWPE]
// updatePositionVecE2: shape [PositionTotalE2, dimensionWPE]
float *updatePositionVecE1, *updatePositionVecE2, *updateMatrixW1PositionE1, *updateMatrixW1PositionE2;
float *updateWordVec;// shape [dimension, wordTotal]

float *bestMatrixB1, *bestMatrixRelation, *bestMatrixW1, *bestMatrixRelationPr;
float *bestPositionVecE1, *bestPositionVecE2, *bestMatrixW1PositionE1, *bestMatrixW1PositionE2;
float *bestWordVec;


string method;

int nowTurn;

map<string,vector<int> > bags_train, bags_test; // map e1-e2-relation to instance id

vector<int> headEntityList, tailEntityList; // kb e1_id e2_id of all train data, indexed by instance id
vector<float> entityVec;       // entity embedding
map<string, int> entityMapping;// map entity to id

// Read word, entity embedding
// Read train and test data
void init() {
    //=============================
    // read word embedding and norm
    //=============================
    string tmpPath = pathString + "data/vec.bin";
    FILE *f = fopen(tmpPath.c_str(), "rb");
    //    FILE *f = fopen("/Users/fengjun/Documents/Research/relation extraction/code/NRE-master/data/vec.bin", "rb");
    tmpPath = pathString + "data/vector1.txt";
    FILE *fout = fopen(tmpPath.c_str(), "w");
    //FILE *fout = fopen("/Users/fengjun/Documents/Research/relation extraction/code/RelationExtraction/data/vector1.txt", "w");
    fscanf(f, "%d", &wordTotal);
    fscanf(f, "%d", &dimension);
    cout<<"wordTotal=\t"<<wordTotal<<endl;
    cout<<"Word dimension=\t"<<dimension<<endl;
    PositionMinE1 = 0;
    PositionMaxE1 = 0;
    PositionMinE2 = 0;
    PositionMaxE2 = 0;
    wordVec = (float *)malloc((wordTotal+1) * dimension * sizeof(float));
    wordList.resize(wordTotal+1);
    wordList[0] = "UNK";
    for (int b = 1; b <= wordTotal; b++) {
        string name = "";
        while (1) {
            char ch = fgetc(f);
            if (feof(f) || ch == ' ') break;
            if (ch != '\n') name = name + ch;
        }
        //        printf("%s\n", name.c_str());
        
        fprintf(fout, "%s", name.c_str());
        int last = b * dimension;
        float smp = 0;
        for (int a = 0; a < dimension; a++) {
            fread(&wordVec[a + last], sizeof(float), 1, f);
            smp += wordVec[a + last]*wordVec[a + last];
        }
        smp = sqrt(smp);
        for (int a = 0; a< dimension; a++)
        {
            wordVec[a+last] = wordVec[a+last] / smp;
            fprintf(fout, "\t%f", wordVec[a+last]);
        }
        fprintf(fout,"\n");
        wordMapping[name] = b;
        wordList[b] = name;
    }
    fclose(fout);
    
    wordTotal+=1;
    fclose(f);
    //=============================
    // read relations
    //=============================
    char buffer[1000];
    tmpPath = pathString + "data/RE/relation2id.txt";
    f = fopen(tmpPath.c_str(), "r");
    //f = fopen("/Users/fengjun/Documents/Research/relation extraction/code/RelationExtraction/data/RE/relation2id.txt", "r");
    while (fscanf(f,"%s",buffer)==1) {
        int id;
        fscanf(f,"%d",&id);
        relationMapping[(string)(buffer)] = id;
        relationTotal++;
        nam.push_back((std::string)(buffer));
    }
    fclose(f);
    cout<<"relationTotal:\t"<<relationTotal<<endl;
    
    //=============================
    // read entities
    //=============================
    tmpPath = pathString + "data/RE/entity2id.txt";
    f = fopen(tmpPath.c_str(), "r");
    //f = fopen("/Users/fengjun/Documents/Research/relation extraction/code/RelationExtraction/data/RE/relation2id.txt", "r");
    while (fscanf(f,"%s",buffer)==1) {
        int id;
        fscanf(f,"%d",&id);
        entityMapping[(string)(buffer)] = id;
    }
    fclose(f);
    printf("entity2id: %d\n", entityMapping.size());

    entityVec.clear();
    tmpPath = pathString + "data/pretrain/entity2vec.txt";// NOTE: different dir
    f = fopen(tmpPath.c_str(), "r");
    float tt;
    while (fscanf(f, "%f", &tt) != EOF)
        entityVec.push_back(tt);
    fclose(f);
    printf("entity2vec: %d\n", entityVec.size()/50);

    //=============================
    // read train and test data
    //=============================
    // data format: e1_kb e2_kb e1_str e2_str relation_str 

    // map e1-e2 to valid relation set w.r.t e1-e2
    map<string, set<int> > trainPair, testPair;
    tmpPath = pathString + "data/RE/train.txt";
    f = fopen(tmpPath.c_str(), "r");
    //f = fopen("/Users/fengjun/Documents/Research/relation extraction/code/RelationExtraction/data/RE/train.txt", "r");
    int trainNum = 0;
    while (fscanf(f,"%s",buffer)==1)  {
        
        string e1 = buffer;
        fscanf(f,"%s",buffer);
        string e2 = buffer;
        
        headEntityList.push_back(entityMapping[e1]);
        tailEntityList.push_back(entityMapping[e2]);
        fscanf(f,"%s",buffer);
        string head_s = (string)(buffer);
        int head = wordMapping[(string)(buffer)];
        fscanf(f,"%s",buffer);
        int tail = wordMapping[(string)(buffer)];
        string tail_s = (string)(buffer);
        fscanf(f,"%s",buffer);
        bags_train[e1+"\t"+e2+"\t"+(string)(buffer)].push_back(headList.size());
        int num = relationMapping[(string)(buffer)];
        trainPair[e1 +"\t"+e2].insert(num);
        
        int len = 0, lefnum = 0, rignum = 0; // entity position
        std::vector<int> tmpp; // store words id in sentence
        while (fscanf(f,"%s", buffer)==1) {
            std::string con = buffer;
            if (con=="###END###") break;
            int gg = wordMapping[con];
            if (con == head_s) lefnum = len;
            if (con == tail_s) rignum = len;
            len++;
            tmpp.push_back(gg);
        }
        
        headList.push_back(head);
        tailList.push_back(tail);
        relationList.push_back(num);
        trainLength.push_back(len);

        // position feature
        int *con=(int *)calloc(len,sizeof(int));
        int *conl=(int *)calloc(len,sizeof(int));
        int *conr=(int *)calloc(len,sizeof(int));
        for (int i = 0; i < len; i++) {
            //word id
            con[i] = tmpp[i];
            conl[i] = lefnum - i;
            conr[i] = rignum - i;
            if (conl[i] >= limit) conl[i] = limit;
            if (conr[i] >= limit) conr[i] = limit;
            if (conl[i] <= -limit) conl[i] = -limit;
            if (conr[i] <= -limit) conr[i] = -limit;
            if (conl[i] > PositionMaxE1) PositionMaxE1 = conl[i];
            if (conr[i] > PositionMaxE2) PositionMaxE2 = conr[i];
            if (conl[i] < PositionMinE1) PositionMinE1 = conl[i];
            if (conr[i] < PositionMinE2) PositionMinE2 = conr[i];
        }
        trainLists.push_back(con);
        trainPositionE1.push_back(conl);
        trainPositionE2.push_back(conr);
        trainNum ++;
        //        if (trainNum >5000) break;
    }
    fclose(f);
    printf("Train data: %d\n", trainNum);
    
    //    for (int i = 0; i < headList.size(); i ++)
    //        printf("%d ", headList[i]);
    //    printf("\n");
    tmpPath = pathString + "data/RE/test.txt";
    f = fopen(tmpPath.c_str(), "r");
    //f = fopen("/Users/fengjun/Documents/Research/relation extraction/code/RelationExtraction/data/RE/test.txt", "r");
    int testNum = 0;
    while (fscanf(f,"%s",buffer)==1)  {
        string e1 = buffer;
        fscanf(f,"%s",buffer);
        string e2 = buffer;
        bags_test[e1+"\t"+e2].push_back(testheadList.size());
        fscanf(f,"%s",buffer);
        string head_s = (string)(buffer);
        int head = wordMapping[(string)(buffer)];
        fscanf(f,"%s",buffer);
        string tail_s = (string)(buffer);
        int tail = wordMapping[(string)(buffer)];
        fscanf(f,"%s",buffer);
        int num = relationMapping[(string)(buffer)];
        testPair[e1+"\t"+e2].insert(num);
        int len = 0 , lefnum = 0, rignum = 0;
        std::vector<int> tmpp;
        while (fscanf(f,"%s", buffer)==1) {
            std::string con = buffer;
            if (con=="###END###") break;
            int gg = wordMapping[con];
            if (head_s == con) lefnum = len;
            if (tail_s == con) rignum = len;
            len++;
            tmpp.push_back(gg);
        }
        testheadList.push_back(head);
        testtailList.push_back(tail);
        testrelationList.push_back(num);
        testtrainLength.push_back(len);
        int *con=(int *)calloc(len,sizeof(int));
        int *conl=(int *)calloc(len,sizeof(int));
        int *conr=(int *)calloc(len,sizeof(int));
        for (int i = 0; i < len; i++) {
            con[i] = tmpp[i];
            conl[i] = lefnum - i;
            conr[i] = rignum - i;
            if (conl[i] >= limit) conl[i] = limit;
            if (conr[i] >= limit) conr[i] = limit;
            if (conl[i] <= -limit) conl[i] = -limit;
            if (conr[i] <= -limit) conr[i] = -limit;
            if (conl[i] > PositionMaxE1) PositionMaxE1 = conl[i];
            if (conr[i] > PositionMaxE2) PositionMaxE2 = conr[i];
            if (conl[i] < PositionMinE1) PositionMinE1 = conl[i];
            if (conr[i] < PositionMinE2) PositionMinE2 = conr[i];
        }
        testtrainLists.push_back(con);
        testPositionE1.push_back(conl);
        testPositionE2.push_back(conr);
        //        if (testNum > 5000) break;
        testNum ++;
    }
    
    //    int trainNumTwo = 0, testNumTwo = 0;
    //    for (map<string,set<int> >:: iterator it = trainPair.begin(); it!=trainPair.end(); it++)
    //    {
    //        if (it -> second.size() > 1)
    //            trainNumTwo ++;
    //    }
    //    for (map<string,set<int> >:: iterator it = testPair.begin(); it!=testPair.end(); it++)
    //    {
    //        if (it -> second.size() > 1)
    //            testNumTwo ++;
    //    }
    //    printf("check data: %d %d\n", trainNumTwo, testNumTwo);
    fclose(f);
    //    cout<<PositionMinE1<<' '<<PositionMaxE1<<' '<<PositionMinE2<<' '<<PositionMaxE2<<endl;
    printf("Test data: %d\n", testNum);

    // fix position feature? 
    for (int i = 0; i < trainPositionE1.size(); i++) {
        int len = trainLength[i];
        int *work1 = trainPositionE1[i];
        for (int j = 0; j < len; j++)
            work1[j] = work1[j] - PositionMinE1;
        int *work2 = trainPositionE2[i];
        for (int j = 0; j < len; j++)
            work2[j] = work2[j] - PositionMinE2;
    }
    
    for (int i = 0; i < testPositionE1.size(); i++) {
        int len = testtrainLength[i];
        int *work1 = testPositionE1[i];
        for (int j = 0; j < len; j++)
            work1[j] = work1[j] - PositionMinE1;
        int *work2 = testPositionE2[i];
        for (int j = 0; j < len; j++)
            work2[j] = work2[j] - PositionMinE2;
    }
    PositionTotalE1 = PositionMaxE1 - PositionMinE1 + 1;
    PositionTotalE2 = PositionMaxE2 - PositionMinE2 + 1;
}

float CalcTanh(float con) {
    if (con > 20) return 1.0;
    if (con < -20) return -1.0;
    float sinhx = exp(con) - exp(-con);
    float coshx = exp(con) + exp(-con);
    return sinhx / coshx;
}

float tanhDao(float con) {
    float res = CalcTanh(con);
    return 1 - res * res;
}

float sigmod(float con) {
    if (con > 20) return 1.0;
    if (con < -20) return 0.0;
    con = exp(con);
    return con / (1 + con);
}

int getRand(int l,int r) {
    int len = r - l;
    int res = rand()*rand() % len;
    if (res < 0)
        res+=len;
    return res + l;
}

float getRandU(float l, float r) {
    float len = r - l;
    float res = (float)(rand()) / RAND_MAX;
    return res * len + l;
}

void norm(float* a, int ll, int rr)
{
    float tmp = 0;
    for (int i=ll; i<rr; i++)
        tmp+=a[i]*a[i];
    if (tmp>1)
    {
        tmp = sqrt(tmp);
        for (int i=ll; i<rr; i++)
            a[i]/=tmp;
    }
}

string Int_to_String(int n)
{
    ostringstream ss;
    ss<<n;
    return ss.str();
}
