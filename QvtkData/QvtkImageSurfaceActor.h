#ifndef __QVTK_IMAGE_SURFACE_ACTOR_H_
#define __QVTK_IMAGE_SURFACE_ACTOR_H_
#pragma once
// me 
#include "QvtkPolyDataActor.h"
namespace Q {
	namespace vtk { 
		class PolyData; 
	}
}
class vtkDiscreteMarchingCubesWithSmooth;
class vtkDiscreteFlyingEdges3D;
class vtkWindowedSincPolyDataFilter;
// vtk
namespace Q {
	namespace vtk {
		class QVTKDATA_EXPORT ImageSurfaceActor : public PolyDataActor
		{
			Q_OBJECT;
			Q_VTK_DATA_H(ImageSurfaceActor);
		public:
			ImageSurfaceActor();
			virtual ~ImageSurfaceActor() override;
			virtual void getPolyData(PolyData *data) const;
			public Q_SLOTS:
			virtual void setRenderDataSet(DataSet *data) override;
		protected:
			virtual Data *newInstance() const override { return new ImageSurfaceActor; }
			vtkDiscreteMarchingCubesWithSmooth *marchingCubes;
			vtkDiscreteFlyingEdges3D *flyingEdge;
			vtkWindowedSincPolyDataFilter *windowedSincPolyDataFilter;
		};
	}
}

#endif // !__QVTK_IMAGE_SURFACE_ACTOR_H_
