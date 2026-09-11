constexpr int dim2lin(int *N,int* i,size_t num) {
	int dumvar = i[num-1];
	if (dumvar < 0)//If negative then I want to revert the index; like python
		dumvar += N[num-1];
    if(num > 1)
    	dumvar += N[num-1]*dim2lin(N,i,num-1);
    return dumvar;
}