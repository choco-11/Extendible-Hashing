// test file to simulate the algorithm
#include <bits/stdc++.h>

#define B 50
#define N 120000
#define BITS 16

using namespace std;

string generateRandomStrings()
{
    string res = "";
    for (int i = 1; i <= 3; i++)
    {
        char ch = 'a' + rand() % 26;
        res = res + ch;
    }
    return res;
}

string hash_val(unsigned short int ID)
{
    int arr[BITS];
    string res;
    for (int i = BITS-1; i >= 0; i--)
    {
        arr[i] = ID % 2;
        res += to_string(arr[i]);
        ID = ID / 2;
    }
    reverse(res.begin(), res.end());
    return res;
}

typedef struct record
{
    unsigned short int id;
    string hash_value;
    string rest;
} record;

typedef struct bucket
{
    int local_depth;
    int size;      // number of records in the bucket
    string prefix; // hash values that it will check
    record record_list[B];
    int overflow_bucket; // pointer to the overflow bucket (in our case the bucket ID)

    bucket()
    {
        local_depth = 0;
        prefix = "";
        size = 0;
        overflow_bucket = -1; // -1 indicate that there is no overflow bucket attached to the current bucket
    }

} bucket;

typedef struct directory
{
    int global_depth;
    vector<string> prefix_list; // will store the hash prefix
    vector<int> bucket_address_list;
} directory;

bucket bucket_memory[N]; // secondary memory
int current_bucket_index = 0;
int overflow_bucket_index = N - 1; // overflow bucket index will start from the end

void insert_record_bucket(bucket &b, record r)
{
    // insert record in the bucket
    b.record_list[b.size] = r;
    b.size++;
}

void display_bucket(int bucket_address)
{
    /* Will display the records in the bucket
     */
    bucket b = bucket_memory[bucket_address];
    while (true)
    {
        cout << "----------------------------------\n";
        cout << "Bucket ID: " << bucket_address << ", Prefix: " << b.prefix << endl;
        printf("Local Depth: %d\n", b.local_depth);
        for (int i = 0; i < b.size; i++)
        {
            printf("ID: %d ", b.record_list[i].id);
            cout<<" => " << b.record_list[i].rest<< endl ;
            cout << "Hash Value: " << b.record_list[i].hash_value << endl;
        }
        // check for the overflow buckets
        if (b.overflow_bucket == -1)
        {
            break;
        }
        bucket_address = b.overflow_bucket;
        b = bucket_memory[bucket_address];
    }
}

void display_status(directory &dir)
{
    /* Will display all the directory and the buckets
     */
    printf("=============================================\n");
    printf("Directory:\n");
    printf("Global Depth: %d\n", dir.global_depth);

    for (int i = 0; i < dir.bucket_address_list.size(); i++)
    {
        int bucket_id = dir.bucket_address_list[i];
        cout << "**********************************\n";
        printf("Directory ID: %d\n", i);
        printf("Prefix: ");
        cout << dir.prefix_list[i] << endl;
        display_bucket(bucket_id);
    }
}

void expand_directory(directory &dir)
{
    dir.global_depth++;

    vector<string> new_prefix_list;
    vector<int> new_bucket_address_list;

    // updating the prefixes
    for (auto prefix : dir.prefix_list)
    {
        new_prefix_list.push_back(prefix + "0");
        new_prefix_list.push_back(prefix + "1");
    }
    dir.prefix_list = new_prefix_list; // ???

    // updating the bucket list
    for (auto bucket_id : dir.bucket_address_list)
    {
        new_bucket_address_list.push_back(bucket_id);
        new_bucket_address_list.push_back(bucket_id);
    }
    dir.bucket_address_list = new_bucket_address_list;
}

int get_bucket(directory &dir, record r)
{
    /* Searches the directory and returns the bucket address in which the record can be inserted
     * */
    string hash_value = r.hash_value;                         // hash value of the record
    string to_check = hash_value.substr(0, dir.global_depth); // check only the MSB/initial binary characters

    int directory_id = -1;
    for (int i = 0; i < dir.prefix_list.size(); i++)
    {
        if (to_check.compare(dir.prefix_list[i]) == 0)
        {
            // got the respective directory ID
            directory_id = i;
            break;
        }
    }

    int bucket_address = dir.bucket_address_list[directory_id];

    return bucket_address;
}

void split_buckets(int bucket_address)
{
    /* Creates an extra bucket
     * */
    current_bucket_index++;                      // creating a new bucket
    bucket_memory[bucket_address].local_depth++; // increasing the local depth
    string previous_prefix = bucket_memory[bucket_address].prefix;
    bucket_memory[bucket_address].prefix = previous_prefix + "0";

    bucket_memory[current_bucket_index].local_depth = bucket_memory[bucket_address].local_depth; // local depth of the new bucket
    bucket_memory[current_bucket_index].prefix = previous_prefix + "1";
}

void rehash_directories(directory &dir, int previous_bucket, int new_bucket)
{
    /* previous_bucket: address of the previous bucket
     * new_bucket: address of the new bucket
     * */

    // only those directories will change who were pointing to the overflow bucket
    vector<int> directory_ids_to_change;
    for (int i = 0; i < dir.bucket_address_list.size(); i++)
    {
        if (dir.bucket_address_list[i] == previous_bucket)
        {
            directory_ids_to_change.push_back(i);
        }
    }

    cout << "Directories to Change:" << endl;
    for (int i : directory_ids_to_change)
    {
        cout << i << " ";
    }
    cout << endl;

    // these directories will either point to the previous bucket or the new bucket
    // Note: this will be determined by the prefix of the bucket and the directory

    string previous_bucket_prefix = bucket_memory[previous_bucket].prefix;

    // We will use the fact that the local depth <= global depth always
    int local_depth = bucket_memory[new_bucket].local_depth;

    for (auto d_id : directory_ids_to_change)
    {
        string to_check = dir.prefix_list[d_id].substr(0, local_depth); // check only the MSB/initial binary characters
        if (to_check.compare(previous_bucket_prefix) == 0)
        {
            // will point the previous bucket
            dir.bucket_address_list[d_id] = previous_bucket;
        }
        else
        {
            // will point to the new bucket
            dir.bucket_address_list[d_id] = new_bucket;
        }
    }
}

void insert_record_overflow_bucket(int bucket_address, record r)
{
    /* bucket_address: bucket address in which the overflow bucket have to be attached
     * */

    // go to the bucket chain which has no overflow bucket
    int overflow_bucket_address = bucket_memory[bucket_address].overflow_bucket;
    while (overflow_bucket_address != -1)
    {
        // travelling like linked list
        bucket_address = overflow_bucket_address;
        overflow_bucket_address = bucket_memory[bucket_address].overflow_bucket;
    }
    // bucket address will now have the address of the bucket in which we have to add the record
    if (bucket_memory[bucket_address].size < B)
    {
        // there is space in the bucket so add it
        insert_record_bucket(bucket_memory[bucket_address], r);
        return;
    }

    // else new overflow bucket have to be made
    overflow_bucket_address = overflow_bucket_index;
    overflow_bucket_index--; // overflow bucket will allocate form the end of the array

    // the overflow bucket will have every thing same (prefix and the local depth)
    bucket b = bucket_memory[bucket_address];
    b.size = 0;
    b.overflow_bucket = -1;
    bucket_memory[overflow_bucket_address] = b;
    insert_record_bucket(bucket_memory[overflow_bucket_address], r);         // insert the record
    bucket_memory[bucket_address].overflow_bucket = overflow_bucket_address; // point to the overflow bucket
}

void rehash_buckets(int previous_bucket, int new_bucket)
{
    /* previous_bucket: address of the previous bucket
     * new_bucket: address of the new bucket
     *
     * ???: Logic for rehashing the overflow bucket
     *      the previous can have multiple overflow buckets
     * */

    vector<record> r_list; // to get all the records in the previous bucket including the overflow bucket
    bucket previous_b = bucket_memory[previous_bucket];
    while (true)
    {
        // TODO: Keep track of the linked buckets which will be useless
        // This is difficult due to the the disk blocks are simulated like an array
        // which means the allocation of the bucket will be difficult

        for (int i = 0; i < previous_b.size; i++)
        {
            // adding the records in the record list
            record r = previous_b.record_list[i];
            r_list.push_back(r);
        }
        // check the linked buckets
        int linked_bucket = previous_b.overflow_bucket;
        if (linked_bucket == -1)
            break;
        previous_b = bucket_memory[linked_bucket];
    }

    bucket_memory[previous_bucket].size = 0; // records will be entered again
    bucket_memory[previous_bucket].overflow_bucket = -1;
    int local_depth = bucket_memory[previous_bucket].local_depth;
    string previous_bucket_prefix = bucket_memory[previous_bucket].prefix;
    // perform rehashing on the records of the previous bucket
    for (record r : r_list)
    {
        string to_compare = r.hash_value.substr(0, local_depth);
        if (to_compare.compare(previous_bucket_prefix) == 0)
        {
            // insert in the previous bucket
            //            insert_record_bucket(bucket_memory[previous_bucket], r);
            insert_record_overflow_bucket(previous_bucket, r);
        }
        else
        {
            // insert the record in new bucket
            insert_record_overflow_bucket(new_bucket, r);
        }
    }

    cout << "Rehash Done" << endl;
}

void insert_record(directory &dir, record r)
{
    /* Will pass through directory
     */

    // fetch the respective bucket
    int bucket_address = get_bucket(dir, r);

    if (bucket_memory[bucket_address].size == B)
    {
        // the bucket if full
        int local_depth = bucket_memory[bucket_address].local_depth;
        // case 1: if local depth <= global depth

        // 0) if global depth = local depth; then expand the directory
        if (dir.global_depth == local_depth)
        {
            expand_directory(dir);
        }

        // 1) split the buckets
        split_buckets(bucket_address);

        // 2) rehash the buckets
        int previous_bucket = bucket_address;
        int new_bucket = current_bucket_index;
        rehash_buckets(previous_bucket, new_bucket);

        // 3) rehash the directories
        rehash_directories(dir, previous_bucket, new_bucket);

        // 4) again attempt to insert the record
        bucket_address = get_bucket(dir, r);
        if (bucket_memory[bucket_address].size == B)
        {
            // make an overflow bucket and insert the record
            insert_record_overflow_bucket(bucket_address, r);
        }
        else
        {
            // overflow bucket not needed
            insert_record_bucket(bucket_memory[bucket_address], r);
        }
    }

    else
    {
        // insert the record in the bucket
        insert_record_bucket(bucket_memory[bucket_address], r);
    }
}

int main()
{

    int records;
    cout << "Enter the number of records: ";
    cin >> records;

    // for random values
    srand(time(0));
    // creating dataset
    ofstream MyFile("dataset.txt");

    for (int i = 1; i <= records; i++)
    {
        MyFile << i << ","                             // Transaction ID
               << 1 + rand() % (500000 - 1 + 1) << "," // Transaction Sale
               << generateRandomStrings() << ","       // Customer Name
               << 1 + rand() % (1500 - 1 + 1) << "\n"; // Category
    }
    MyFile.close();


    // initial configurations
    bucket inital_bucket;
    bucket_memory[0] = inital_bucket;
    directory dir;
    dir.global_depth = 0;
    dir.prefix_list.push_back("");
    dir.bucket_address_list.push_back(current_bucket_index);

    ifstream ReadID("dataset.txt");
    unsigned short int ID;
    char Info[20];
    for (int i = 1; i <= records; i++)
    {
        string buffer;  // !!! please don't remove this line
        getline(ReadID, buffer);
        sscanf((char *)&buffer[0], "%hu,%s[^,]", &ID, Info);
        // cout << ID << " => " << Info << endl;

        record obj;
        obj.id = ID;
        obj.hash_value = hash_val(ID);
        obj.rest = Info;
        insert_record(dir,obj);
    }
    ReadID.close();
    cout << "\n";
    display_status(dir);


    return 0;
}