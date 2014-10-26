// ChildView.cpp
//

#include "stdafx.h"
#include "DelaunayDemo.h"
#include "ChildView.h"
#include "QPerformanceTimer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

const int range = 1000;
#define colVertex	(RGB(255, 128, 0))

#if(WINVER >= 0x0400)
#define FONT	DEFAULT_GUI_FONT
#else
#define FONT	SYSTEM_FONT
#endif

inline int Int(REAL r)	{ return (int) floor(r + 0.5f); }	// properly round of a REAL

// Function object to draw a Vertex; to be used in for_each algorithm.
class drawVertex
{
public:
	drawVertex(CDC& dc, REAL hScale, REAL vScale) : m_DC(dc), m_hScale(hScale), m_vScale(vScale)	{}
	void operator()(const Vertex& v) const
	{
		const int halfSize = 2;

		CRect rc;
		rc.SetRectEmpty();
		rc.InflateRect(halfSize, halfSize);
		rc.OffsetRect(Int(m_hScale * v.GetX()), Int(m_vScale * v.GetY()));
		m_DC.FillSolidRect(& rc, colVertex);
	}
protected:
	CDC& m_DC;
	REAL m_hScale;
	REAL m_vScale;
};

// Function object to draw an edge.
class drawEdge
{
public:
	drawEdge(CDC& dc, REAL hScale, REAL vScale) : m_DC(dc), m_hScale(hScale), m_vScale(vScale)	{}
	void operator()(const edge& e) const
	{
		m_DC.MoveTo(Int(m_hScale * e.m_pV0->GetX()), Int(m_vScale * e.m_pV0->GetY()));
		m_DC.LineTo(Int(m_hScale * e.m_pV1->GetX()), Int(m_vScale * e.m_pV1->GetY()));
	}
protected:
	CDC& m_DC;
	REAL m_hScale;
	REAL m_vScale;
};

// Function object to draw a triangle.
class drawTriangle
{
public:
	drawTriangle(CDC& dc, REAL hScale, REAL vScale) : m_DC(dc), m_hScale(hScale), m_vScale(vScale)	{}
	void operator()(const triangle& tri) const
	{
		const Vertex * v0 = tri.GetVertex(0);
		m_DC.MoveTo(Int(m_hScale * v0->GetX()), Int(m_vScale * v0->GetY()));
		const Vertex * v1 = tri.GetVertex(1);
		m_DC.LineTo(Int(m_hScale * v1->GetX()), Int(m_vScale * v1->GetY()));
		const Vertex * v2 = tri.GetVertex(2);
		m_DC.LineTo(Int(m_hScale * v2->GetX()), Int(m_vScale * v2->GetY()));
		m_DC.LineTo(Int(m_hScale * v0->GetX()), Int(m_vScale * v0->GetY()));
	}
protected:
	CDC& m_DC;
	REAL m_hScale;
	REAL m_vScale;
};

////////////////////
// CChildView

CChildView::CChildView()
: m_nVertices(20)
, m_nTime(0)
{
}

CChildView::~CChildView()
{
}

BEGIN_MESSAGE_MAP(CChildView, CWnd)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_COMMAND(ID_FILE_OPTIONS, OnFileOptions)
	ON_COMMAND(ID_UPDATE, &CChildView::OnUpdate)
	ON_COMMAND(ID_UPDATEPOINT, &CChildView::OnUpdatepoint)
END_MESSAGE_MAP()

BOOL CChildView::PreCreateWindow(CREATESTRUCT& cs) 
{
	if (!CWnd::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, 
		::LoadCursor(NULL, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW+1), NULL);

	return TRUE;
}

void CChildView::OnPaint() 
{
	this->SetFocus();
	CPaintDC dc(this);
	dc.SetViewportOrg(m_hMargin, m_vMargin);

//	for_each(m_Triangles.begin(), m_Triangles.end(), drawTriangle(dc, m_hScale, m_vScale));		// for debugging
	m_router.drawTriangles(dc, m_hScale, m_vScale);

	// If we simply draw all the triangles, most of the edges are drawn twice.
	// Therefore, we first eliminate the superfluous edges.
// 	edgeSet edges;
// 	Delaunay d;
// 	d.TrianglesToEdges(m_Triangles, edges);
// 
// 	for_each(edges.begin(), edges.end(), drawEdge(dc, m_hScale, m_vScale));
// 	for_each(m_Vertices.begin(), m_Vertices.end(), drawVertex(dc, m_hScale, m_vScale));
// 
// 	CGdiObject * pOldFont = dc.SelectStockObject(FONT);
// 	int oldBkMode = dc.SetBkMode(TRANSPARENT);

// 	CString s;
// 	s.Format(IDS_FORMAT, m_nVertices, m_nTime);
// 	dc.TextOut(0, m_yText, s);
// 
// 	dc.SetBkMode(oldBkMode);
// 	if (pOldFont) dc.SelectObject(pOldFont);
}

void CChildView::OnSize(UINT nType, int cx, int cy)
{
	const int marginFactor = 20;

	CWnd::OnSize(nType, cx, cy);
	if (cx == 0 || cy == 0) return;

	CClientDC dc(this);
	CGdiObject * pOldFont = dc.SelectStockObject(FONT);
	CSize sz = dc.GetTextExtent(_T("M"), 1);
	if (pOldFont) dc.SelectObject(pOldFont);

	m_hMargin = cx / marginFactor;
	m_vMargin = cy / marginFactor;
	int w = cx - 2 * m_hMargin;
	int h = cy - 2 * m_vMargin - sz.cy;
	m_hScale = (REAL) w / (REAL) range;
	m_vScale = (REAL) h / (REAL) range;
	m_hScale = m_vScale;

	m_yText = h + sz.cy / 2;
}

int CChildView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	NewVertices();
	return 0;
}
void CChildView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	Invalidate();
}
void CChildView::OnMouseMove(UINT nFlags, CPoint point)
{

	Invalidate();
}
void CChildView::NewVertices(void)
{
	// Fill m_Vertices with a new set of random vertices and calculate the Delaunay-triangulation.

	m_Vertices.clear();
	m_Triangles.clear();


	m_router.reset();
	const int radiusRange = 40;
	const int padding  = range * 0.14;
	m_nVertices = 12;
	for (int ithCon = 0; ithCon < 50; ithCon++)
	{
		vector<VtxPoint> pnts;
		double radiusX = rand() % radiusRange+1;
		double radiusY = radiusX;//rand() % radiusRange+1;
		int pntRange = range - padding * 2;
		double cx = rand() % pntRange + padding;
		double cy = rand() % pntRange + padding;
		for (int i = 0; i <= m_nVertices; ++i)
		{
			double f = ((rand() / 65535.0) - 0.5) * 0.0 + 1;
			double theta = (double)i / m_nVertices * 2 * M_PI;
			double x = cx + cos(theta) * (f * radiusX);
			double y = cy + sin(theta) * (f * radiusY);
			pnts.push_back(VtxPoint(x,y));
		}
		m_router.addContour(pnts);
	}
	m_router.compute();
	OpenMesh::IO::write_mesh(m_router.getRouteMesh(), "mesh.obj");
// 	for (int i = 0; i < m_nVertices; i++)
// 	{
// 		int x = rand() % range;
// 		int y = rand() % range;
// 		m_Vertices.insert(Vertex(x, y));
// 	}
// 
// 	{	// Block defines measuring time.
// 		QPerformanceTimer timer(m_nTime);
// 
// 		Delaunay d;
// 		d.Triangulate(m_Vertices, m_Triangles);
// 	}
}

void CChildView::OnFileOptions()
{
	COptionsDlg dlg;
	dlg.m_nVertices = m_nVertices;
	if (dlg.DoModal() == IDOK)
	{
		m_nVertices = dlg.m_nVertices;
		NewVertices();
		Invalidate();
	}
}

/////////////////////
// COptionsDlg

IMPLEMENT_DYNAMIC(COptionsDlg, CDialog)
COptionsDlg::COptionsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(COptionsDlg::IDD, pParent)
	, m_nVertices(0)
{
}

COptionsDlg::~COptionsDlg()
{
}

void COptionsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_NVERTICES, m_nVertices);
	DDV_MinMaxInt(pDX, m_nVertices, 0, 2000);
	DDX_Control(pDX, IDC_SNVERICES, c_spinVertices);
}

BEGIN_MESSAGE_MAP(COptionsDlg, CDialog)
END_MESSAGE_MAP()

BOOL COptionsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	c_spinVertices.SetRange(0, 2000);
	return TRUE;
}

void CChildView::OnUpdate()
{
	// TODO: 在此添加命令处理程序代码
	Invalidate();
}

void CChildView::OnUpdatepoint()
{
	// TODO: 在此添加命令处理程序代码
	NewVertices();
	Invalidate();
}
