#include "stdafx.h"
#include "stressMDSLib.h"

namespace CodeAtlas{
namespace StressMDSLib
{

Matrix<double> * MDS_UCF(Matrix<double> * D, Matrix<double> * X0,int dim, int iter, double * weight, double distWeightPwr)
{
	if(D->rows()!=D->columns())
	{
		printf("Input distance matrix to MDS is not square.\n");
		exit(1);
	}
	if(dim<1)
	{
		printf("Invalid dimension for MDS.\n");
		exit(1);
	}
	if(iter<1)
	{
		printf("Invalid number of iterations for MDS.\n");
		exit(1);
	}

	Matrix<double> * X=NULL;		
	// with initialization
	if(X0!=NULL)
	{
		if(X0->rows()!=D->rows() || X0->columns()!=dim)
		{
			printf("Input initialization to MDS has invalid dimension.\n");
			exit(1);
		}
		X=X0->copy();
	}
	// without initialization
	else
	{
		X=new Matrix<double>(D->rows(),dim,"rand");
		double D_mean=D->mean(); // mean value of distance matrix
		X->addNumberSelf(-0.5); // move to the center
		X->multiplyNumberSelf(0.1*D_mean/(1.0/3.0*sqrt((double)dim))); // before this step, mean distance is 1/3*sqrt(d)
	}


	double lr=0.05; // learning rate
	double r=2; // metric
	int n=D->rows(); // number of vectors		

	Matrix<double> * dPos2DLengthMat=new Matrix<double>(n,n,0.0);
	Matrix<double> * dPos2D=new Matrix<double>(n,dim);
	Matrix<double> * dPos2DLengthVec=new Matrix<double>(n,1);
	Matrix<double> * dispMat=new Matrix<double>(n-1,dim,0.0);
	// array that stores the weight of each pair distance,
	// distWeightMat(i,j) = (D(i,j) / meanD) ^ distWeightPwr
	Matrix<double> * distWeightMat=new Matrix<double>(n,n);

	Matrix<int> * RP=new Matrix<int>(n,iter,"randperm"); // the matrix for random permutation numbers

	double* w = new double[D->rows()];
	const float meanD = D->mean();
	for (int i = 0; i < D->rows(); ++i)
	{
		w[i] = (weight == NULL) ? 1.0 : weight[i];	
		for (int j = 0; j < D->columns(); ++j)
		{
			int idx = i * D->columns() + j;
			float distW = pow((D->get(i,j) + 1) / meanD, distWeightPwr);
			//qDebug("%d , %d , w = %f", i,j,distW);
			distWeightMat->set(i,j, distW);
		}			
	}		

	int i,j;
	double temp;
	int m;

	printf("MDS iteration:"); 
	for(int it=0;it<iter;it++) // iterations
	{
		if(it%10==0) printf("\n");
		printf("%3d  ",it+1); 
		for(int rp=0;rp<n;rp++) // work on each vector in a randomly permuted order
		{
			m=RP->get(rp,it)-1;			// m is the index of current processing point

			// dPos2D = pos2D(m) - pos2D(other points)
			for(i=0;i<n;i++)
			{
				for(j=0;j<dim;j++)
				{
					dPos2D->set(i,j,  X->get(m,j)-X->get(i,j)  );
				}
			}

			// dPos2DLengthVec = length of each row of pmat
			//                 = distances between m and other points
			for(i=0;i<n;i++)
			{
				temp=0;
				for(j=0;j<dim;j++)
				{
					temp+= pow(fabs(dPos2D->get(i,j)), r);
				}
				dPos2DLengthVec->set(i,0,  pow(temp,1/r) + 1e-10 );		// prevent divided by 0
			}

			// dPos2DLengthMat = all-pair distance of 2D points
			for(i=0;i<n;i++)
			{
				if(i==m) continue;

				dPos2DLengthMat->set(m,i,  dPos2DLengthVec->get(i,0)  );
				dPos2DLengthMat->set(i,m,  dPos2DLengthVec->get(i,0)  );
			}

			// dispMat = other points' displacement
			for(i=0;i<n-1;i++)
			{
				int ii=i;
				if(i>=m) ii=i+1;
				// temp = learn rate * (actual distance - desired distance) / (actual distance)
				temp=lr * (dPos2DLengthVec->get(ii,0) - D->get(ii,m)) * pow(dPos2DLengthVec->get(ii,0), 1-r) ;//* distWeightMat->get(ii,m);
				for(j=0;j<dim;j++)
				{
					dispMat->set(i,j,  temp  );
				}
			}

			// move point m and other points
			float curWeight = w[m];
			for(i=0;i<n-1;i++)
			{
				int ii=i;
				if(i>=m) ii=i+1;

				float otherWeight = w[ii];
				float curDisp     = otherWeight / (curWeight + otherWeight);
				float otherDisp   = curWeight   / (curWeight + otherWeight);

				for(j=0;j<dim;j++)
				{
					float totDisp = dispMat->get(i,j) * pow(fabs( dPos2D->get(ii,j) ),r-1) * sign<double>(dPos2D->get(ii,j));
					X->getRef(ii,j) += totDisp * otherDisp;
					X->getRef(m,j)  -= totDisp * curDisp;
				}
			}
		}
	}

	printf("\n");

	delete dPos2DLengthMat;
	delete dPos2D;
	delete dPos2DLengthVec;
	delete dispMat;
	delete distWeightMat;
	delete RP;
	delete[] w;

	return X;
}

}
}