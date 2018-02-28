struct point
{
    int x;
    int y;
};
void mmain()
{
    int arr[10];
    int i;
    struct point pt;
    pt.x=1024;
    pt.y=768;
    for (int i=0;i<10;i++)
    {
        arr[i]=i;
        if(i==6)
        {
            continue;
        }else{
            printf("a[%d]=%d\n",i,arr[i]);
        }
    }
    printf("pt.x=%d,pt.y=%d\n",pt.x,pt.y);
}

