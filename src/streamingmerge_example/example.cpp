//element = tuple <string, int>
//use linked lists instead of arrays (?)
//dynamically malloc arrays (so we don't have to predtermine size)
// Look into non min heap implementation
// given two sorted packets, linear pass through each of them and populate a larger list


// C++ program to merge k sorted arrays of size n each.
// Predefined number of arrays that we can define, but we pass in the number of elements per array.
// We merge sort each array, then use a Minheap to merge these sorted arrays. 
// Passing in arrays of tuples, sorting is taking place on the index value of each tuple
#include<iostream> 
#include<limits.h> 
#include<list>
#include<iterator>
using namespace std; 

#define n 1000

//linked list node struct
struct list_node{
	struct test_tuple data;
	struct list_node* next;
};


//Struct tuple that will populate our linked lists
struct test_tuple{
	string data;
	int index;
};


// A min heap node 
struct MinHeapNode 
{ 
	struct test_tuple element; // The element to be stored 
	int i; // index of the array from which the element is taken 
	int j; // index of the next element to be picked from array 
}; 

// Prototype of a utility function to swap two min heap nodes 
void swap(MinHeapNode *x, MinHeapNode *y); 

// A class for Min Heap 
class MinHeap 
{ 
	MinHeapNode *harr; // pointer to array of elements in heap 
	int heap_size; // size of min heap 
public: 
	// Constructor: creates a min heap of given size 
	MinHeap(MinHeapNode a[], int size); 

	// to heapify a subtree with root at given index 
	void MinHeapify(int ); 

	// to get index of left child of node at index i 
	int left(int i) { return (2*i + 1); } 

	// to get index of right child of node at index i 
	int right(int i) { return (2*i + 2); } 

	// to get the root 
	MinHeapNode getMin() { return harr[0]; } 

	// to replace root with new node x and heapify() new root 
	void replaceMin(MinHeapNode x) { harr[0] = x; MinHeapify(0); } 
}; 

// This function takes an array of arrays as an argument and 
// All arrays are assumed to be sorted. It merges them together 
// and prints the final sorted output. 
struct test_tuple *mergeKArrays(struct test_tuple arr[][n], int k) 
{ 
	struct test_tuple *output = new struct test_tuple[n*k]; // To store output array 

	// Create a min heap with k heap nodes. Every heap node 
	// has first element of an array 
	MinHeapNode *harr = new MinHeapNode[k]; 
	for (int i = 0; i < k; i++) 
	{ 
		harr[i].element.index = arr[i][0].index; // Store the first element 
		harr[i].i = i; // index of array 
		harr[i].j = 1; // Index of next element to be stored from array 
	} 
	MinHeap hp(harr, k); // Create the heap 

	// Now one by one get the minimum element from min 
	// heap and replace it with next element of its array 
	for (int count = 0; count < n*k; count++) 
	{ 
		// Get the minimum element and store it in output 
		MinHeapNode root = hp.getMin(); 
		output[count].index = root.element.index; 

		// Find the next elelement that will replace current 
		// root of heap. The next element belongs to same 
		// array as the current root. 
		if (root.j < n) 
		{ 
			root.element.index = arr[root.i][root.j].index; 
			root.j += 1; 
		} 
		// If root was the last element of its array 
		else root.element.index = INT_MAX; //INT_MAX is for infinite 

		// Replace root with next element of array 
		hp.replaceMin(root); 
	} 

	return output; 
} 

// FOLLOWING ARE IMPLEMENTATIONS OF STANDARD MIN HEAP METHODS 
// FROM CORMEN BOOK 
// Constructor: Builds a heap from a given array a[] of given size 
MinHeap::MinHeap(MinHeapNode a[], int size) 
{ 
	heap_size = size; 
	harr = a; // store address of array 
	int i = (heap_size - 1)/2; 
	while (i >= 0) 
	{ 
		MinHeapify(i); 
		i--; 
	} 
} 

// A recursive method to heapify a subtree with root at given index 
// This method assumes that the subtrees are already heapified 
void MinHeap::MinHeapify(int i) 
{ 
	int l = left(i); 
	int r = right(i); 
	int smallest = i; 
	if (l < heap_size && harr[l].element.index < harr[i].element.index) 
		smallest = l; 
	if (r < heap_size && harr[r].element.index < harr[smallest].element.index) 
		smallest = r; 
	if (smallest != i) 
	{ 
		swap(&harr[i], &harr[smallest]); 
		MinHeapify(smallest); 
	} 
} 

// A utility function to swap two elements 
void swap(MinHeapNode *x, MinHeapNode *y) 
{ 
	MinHeapNode temp = *x; *x = *y; *y = temp; 
} 

// A utility function to print array elements 
void printArray(struct test_tuple arr[], int size) 
{ 
for (int i=0; i < size; i++) 
	cout << arr[i].index << " "; 
} 

int Merge(struct test_tuple A[],int p, int q,int r)     
{

    int n1,n2,i,j,k; 
    //size of left array=n1
    //size of right array=n2       
    n1=q-p+1;
    n2=r-q;             
    int L[n1],R[n2];
    //initializing the value of Left part to L[]
    for(i=0;i<n1;i++)
    {
        L[i]=A[p+i].index;
    }
    //initializing the value of Right Part to R[]
    for(j=0;j<n2;j++)
    {
        R[j]=A[q+j+1].index;
    }
    i=0,j=0;
    //Comparing and merging them
    //into new array in sorted order 
    for(k=p;i<n1&&j<n2;k++)
    {
        if(L[i]<R[j])
        {
            A[k].index=L[i++];
        }
        else
        {
            A[k].index=R[j++];
        }
    }
    //If Left Array L[] has more elements than Right Array R[]
    //then it will put all the
    // reamining elements of L[] into A[]
    while(i<n1)             
    {
        A[k++].index=L[i++];
    }
    //If Right Array R[] has more elements than Left Array L[]
    //then it will put all the
    // reamining elements of L[] into A[]
    while(j<n2)
    {
        A[k++].index=R[j++];
    }
}
//This is Divide Part
//This part will Divide the array into 
//Sub array and then will Merge them
//by calling Merge()
int MergeSort(struct test_tuple A[],int p,int r)    
{
    int q;                                
    if(p<r)
    {
        q=(p+r)/2;
        MergeSort(A,p,q);
        MergeSort(A,q+1,r);
        Merge(A,p,q,r);
    }
}




// Driver program to test above functions 
int main(int argc, char* argv[]) 
{ 
	// Change n at the top to change number of elements 
	// in an array 
	// number of n element arrays is provided as a command line arg.
    if(argc == 2){
        int k = atoi(argv[1]);

        struct test_tuple arr[k][n];
        for(int i = 0; i < k; i++){
            for(int j =0; j < n;j++){
                arr[i][j].index = rand() % (n*k);
            }
            MergeSort(arr[i],0,n-1);
        }
        struct test_tuple *output = mergeKArrays(arr, k); 
	    cout << "Merged array is " << endl; 
	    printArray(output, n*k); 
        return 0;

    }
    else
        printf("No second arg detected. Going with predefined case.\n");

	return 0; 
} 
