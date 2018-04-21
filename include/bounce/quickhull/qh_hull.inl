// qhHull.h

// Lists

template<class T>
inline void qhList<T>::PushFront(T* link)
{
	link->prev = NULL;
	link->next = head;
	if (head)
	{
		head->prev = link;
	}
	head = link;
	++count;
}

template<class T>
inline T* qhList<T>::Remove(T* link)
{
	T* next = link->next;

	if (link->prev)
	{
		link->prev->next = link->next;
	}
	if (link->next)
	{
		link->next->prev = link->prev;
	}
	if (link == head)
	{
		head = link->next;
	}

	link->prev = NULL;
	link->next = NULL;

	--count;
	return next;
}

// qhFace

inline u32 qhFace::GetVertexCount() const
{
	u32 count = 0;
	qhHalfEdge* e = edge;
	do
	{
		++count;
		e = e->next;
	} while (e != edge);
	return count;
}

inline u32 qhFace::GetEdgeCount() const
{
	u32 count = 0;
	qhHalfEdge* e = edge;
	do
	{
		++count;
		e = e->next;
	} while (e != edge);
	return count;
}

inline qhHalfEdge* qhFace::FindTwin(const qhVertex* tail, const qhVertex* head) const
{
	qhHalfEdge* e = edge;
	do
	{
		qhVertex* tail2 = e->tail;
		qhVertex* head2 = e->next->tail;

		if (tail2 == tail && head2 == head)
		{
			return e;
		}

		e = e->next;
	} while (e != edge);

	return NULL;
}

static inline b3Vec3 b3Newell(const b3Vec3& a, const b3Vec3& b)
{
	return b3Vec3((a.y - b.y) * (a.z + b.z), (a.z - b.z) * (a.x + b.x), (a.x - b.x) * (a.y + b.y));
}

inline void qhFace::ComputeCenterAndPlane()
{
	b3Vec3 c;
	c.SetZero();

	b3Vec3 n;
	n.SetZero();

	u32 count = 0;
	qhHalfEdge* e = edge;
	do
	{
		b3Vec3 v1 = e->tail->position;
		b3Vec3 v2 = e->next->tail->position;

		n += b3Newell(v1, v2);
		c += v1;

		++count;
		e = e->next;
	} while (e != edge);

	B3_ASSERT(count > 0);
	c /= float32(count);
	n.Normalize();
	plane.normal = n;
	plane.offset = b3Dot(n, c);
	center = c;
}

// qhHull

// Given a number of points return the required memory size in 
// bytes for constructing the convex hull of those points. 
// This function uses constant expression (C++11). Therefore, you can evaluate 
// its value at compile-time. That is particularly usefull when you want to 
// create a stack buffer from a constant number of vertices. 
// Due to the constexpr specifier, this function is automatically inlined.
#if defined(__cpp_constexpr) && __cpp_constexpr > 200704
  constexpr //relaxed constexpr
#else
  inline
#endif
u32 qhGetBufferSize(u32 pointCount)
{
	u32 size = 0;

	// Hull using Euler's Formula
	u32 V = pointCount;
	u32 E = 3 * V - 6;
	u32 HE = 2 * E;
	u32 F = 2 * V - 4;
	
	size += V * sizeof(qhVertex);
	size += HE * sizeof(qhHalfEdge);
	size += F * sizeof(qhFace);

	// Extra
	size += HE * sizeof(qhHalfEdge);
	size += F * sizeof(qhFace);

	// Horizon
	size += HE * sizeof(qhHalfEdge*);
	
	// New Faces
	// One face per horizon edge
	size += HE * sizeof(qhFace*);
	
	return size;
}

inline u32 qhHull::GetIterations() const
{
	return m_iteration;
}

inline const qhList<qhFace>& qhHull::GetFaceList() const
{
	return m_faceList;
}

inline qhVertex* qhHull::AllocateVertex()
{
	qhVertex* v = m_freeVertices;
	m_freeVertices = v->freeNext;
	return v;
}

inline void qhHull::FreeVertex(qhVertex* v)
{
	v->freeNext = m_freeVertices;
	m_freeVertices = v;
}

inline qhHalfEdge* qhHull::AllocateEdge()
{
	qhHalfEdge* e = m_freeEdges;
	m_freeEdges = e->freeNext;
	return e;
}

inline void qhHull::FreeEdge(qhHalfEdge* e)
{
	e->freeNext = m_freeEdges;
	m_freeEdges = e;
}

inline qhFace* qhHull::AllocateFace()
{
	qhFace* f = m_freeFaces;
	m_freeFaces = f->freeNext;
	return f;
}

inline void qhHull::FreeFace(qhFace* f)
{
	f->state = qhFace::e_deleted;
	f->freeNext = m_freeFaces;
	m_freeFaces = f;
}

inline qhHalfEdge* qhHull::FindTwin(const qhVertex* tail, const qhVertex* head) const
{
	qhFace* face = m_faceList.head;
	while (face)
	{
		qhHalfEdge* e = face->FindTwin(tail, head);
		if (e)
		{
			return e;
		}
		face = face->next;
	}
	return NULL;
}
