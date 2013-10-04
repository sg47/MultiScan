#include "PostProcessor.h"
#include "scanner3dlib.h"

extern ScannerAlg *pScanner;
PostProcessor::PostProcessor(void)
{
}

PostProcessor::~PostProcessor(void)
{
}

/*
This is a powerful post-proccessing function,
what it does is this:
Every 3d point generated by the ScannerAlg corresponds
directly to 1 2d point from the scanned image. 
This algorithm creates an X*Y map, where X and Y is the image size.
Each 3d point is sorted into it's correct spot.
Then, each point is merged (averaged) to reduce jitter associated
with multiple scans. This will vastly reduce the number of points down 
to ImageXSize*ImageYSize or less, before running this alg, the potential 
max number of points is ImageXSize * ImageYSize * #Scanned Frames.
*/
void PostProcessor::Merge(List *outlst)
{
	int width;
	int height;
	if(ImProc::Instance()->GetReference() == 0)
		return;
	width = ImProc::Instance()->GetReference()->width;
	height = ImProc::Instance()->GetReference()->height;
	//create some storage for this
	List *map = new List[width * height];
	for(int c=0;c<(width*height);c++)
		map[c].Init();
	List composited;
	Composite(&composited);
	//iterate through all the points in all scanned frames
	for (ListItem *li = composited.list ; li != 0 ; li=li->next)
	{
		//Now add it to the correct bucket.
		point_3d *pnt = (point_3d *)li->data;
		map[pnt->m_p2d.Y * width + pnt->m_p2d.X].Add(pnt);
	}
	//now all the points are correctly sorted in thier buckets

	//walk through each and every position and create a new point that is the average in that X/Y spot
	float tx,ty,tz;
	int numpoints;
	for(int y = 0; y < height; y++)
	{
		for(int x = 0; x < width; x++)
		{
			tx = 0.0f;
			ty = 0.0f;
			tz = 0.0f;
			numpoints = map[y * width + x].Count();
			if(numpoints > 0) // if not an empty list
			{
				//iterate through all the points at this position
				Color clr;
				for(ListItem *li = map[y * width + x].list; li != 0; li = li->next)
				{
					point_3d *tmp = (point_3d *)li->data;
					tx += tmp->Wx;
					ty += tmp->Wy;
					tz += tmp->Wz;
					clr = tmp->m_color;
				}
				//create a new point
				point_3d *newpnt = new point_3d();
				//average the values
				newpnt->Wx = tx / (float)numpoints;
				newpnt->Wy = ty / (float)numpoints;
				newpnt->Wz = tz / (float)numpoints;
				newpnt->m_color = clr;
				//save it to the output list
				outlst->Add(newpnt);
			}
		}
	}
	delete []map;

}
/*
Composite does not create any new points,
it just gathers them up from the scannerframes
*/
void PostProcessor::Composite(List *outlst)
{	
	for (ListItem *li = pScanner->m_pFrames->list ; li != 0 ; li=li->next)
	{
		ScannerFrame *sf = (ScannerFrame *)li->data;
		for(ListItem *li2 = sf->m_pPoints->list ; li2 !=0 ; li2 = li2->next)
		{			
			point_3d *pnt = (point_3d *)li2->data;
			outlst->Add(pnt);	
		}
	}	
}

void PostProcessor::SaveData(char * filename, List *lstpnts)
{
	FILE *fp = fopen(filename,"wb");

	fprintf(fp,"ply\r\n");
	fprintf(fp,"format ascii 1.0\r\n");
	fprintf(fp,"element vertex %d\r\n",lstpnts->Count());
	fprintf(fp,"property float x\r\n");
	fprintf(fp,"property float y\r\n");
	fprintf(fp,"property float z\r\n");
	fprintf(fp,"property uchar diffuse_red\r\n");
	fprintf(fp,"property uchar diffuse_green\r\n");
	fprintf(fp,"property uchar diffuse_blue\r\n");
	fprintf(fp,"element face 0\r\n");
	fprintf(fp,"property list uchar int vertex_indices\r\n");
	fprintf(fp,"end_header\r\n");

	for(ListItem *li = lstpnts->list; li!=0; li=li->next)
	{
		point_3d *pnt = (point_3d *)li->data;
		fprintf(fp,"%f %f %f %d %d %d\r\n",pnt->Wx,pnt->Wy,pnt->Wz,pnt->m_color.R,pnt->m_color.G,pnt->m_color.B);
	}

	fclose(fp);
}