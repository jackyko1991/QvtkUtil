#include "QvtkOrthogonalViewer.h"
#include "ui_QvtkOrthogonalViewer.h"
//vtk
#include <vtkCamera.h>
#include <vtkOpenGLCamera.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <QVTKInteractor.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkMath.h>
//qt
#include <QLayout>
#include <QDebug>
const struct QvtkOrthogonalViewerResourceInit
{
	QvtkOrthogonalViewerResourceInit() {
		Q_INIT_RESOURCE(QvtkOrthogonalViewer);
	}
} INIT;
namespace Q {
	namespace vtk {
		/* set default value of x and y view plane normal */
		bool righthandness_ = true;
		double sagittalViewPlaneNormal_[3] = { 1, 0, 0 };
		double coronalViewPlaneNormal_[3] = { 0, 1, 0 };
		double axialViewPlaneNormal_[3] = { 0, 0, 1 };
		const double OrthogonalViewer::ORIENTATION_YZ_NORMAL[3] = { 1, 0, 0 };
		const double OrthogonalViewer::ORIENTATION_XZ_NORMAL[3] = { 0, 1, 0 };
		const double OrthogonalViewer::ORIENTATION_XY_NORMAL[3] = { 0, 0, 1 };
		void OrthogonalViewer::setOrientation(int orientation)
		{
			ORIENTATION _orientation = static_cast<ORIENTATION>(orientation);
			switch (_orientation)
			{
			case OrthogonalViewer::ORIENTATION_YZ:
				this->ui->pushButtonYZ->setChecked(true);
				break;
			case OrthogonalViewer::ORIENTATION_XZ:
				this->ui->pushButtonXZ->setChecked(true);
				break;
			case OrthogonalViewer::ORIENTATION_XY:
				this->ui->pushButtonXY->setChecked(true);
				break;
			case OrthogonalViewer::AXIAL:
				this->ui->pushButtonAxial->setChecked(true);
				break;
			case OrthogonalViewer::CORONAL:
				this->ui->pushButtonCoronal->setChecked(true);
				break;
			case OrthogonalViewer::SAGITTAL:
				this->ui->pushButtonSagittal->setChecked(true);
				break;
			default:
				break;
			}
			this->orientation = _orientation;
			emit OrientationChanged(this->orientation);
			this->UpdateCameraViewPlaneNormal();
			//this->getkRenderWindow()->Render();
			this->update();
		}

		OrthogonalViewer::OrthogonalViewer(QWidget *parent)
			: Viewer(parent)
		{
			this->ui = new Ui::OrthogonalViewer;
			this->ui->setupUi(this);
			vtkNew<vtkGenericOpenGLRenderWindow> renderWindow;
			this->ui->qvtkWidget->SetRenderWindow(renderWindow);
			this->viewPlaneNormalSyncFlag = true;
			this->righthandness = true;
			this->orientation = ORIENTATION_XY;
			std::copy(ORIENTATION_XY_NORMAL, ORIENTATION_XY_NORMAL + 3, this->currentPlaneNormal);
			std::copy(sagittalViewPlaneNormal_, sagittalViewPlaneNormal_ + 3, this->sagittalViewPlaneNormal);
			std::copy(coronalViewPlaneNormal_, coronalViewPlaneNormal_ + 3, this->coronalViewPlaneNormal);
			std::copy(axialViewPlaneNormal_, axialViewPlaneNormal_ + 3, this->axialViewPlaneNormal);
			this->setupFirstRenderer();
			// connection 
			connect(this->ui->pushButtonAxial, &QPushButton::clicked, this, &OrthogonalViewer::setOrientationToAxial);
			connect(this->ui->pushButtonCoronal, &QPushButton::clicked, this, &OrthogonalViewer::setOrientationToCoronal);
			connect(this->ui->pushButtonSagittal, &QPushButton::clicked, this, &OrthogonalViewer::setOrientationToSagittal);
			connect(this->ui->pushButtonYZ, &QPushButton::clicked, this, &OrthogonalViewer::setOrientationToYZ);
			connect(this->ui->pushButtonXZ, &QPushButton::clicked, this, &OrthogonalViewer::setOrientationToXZ);
			connect(this->ui->pushButtonXY, &QPushButton::clicked, this, &OrthogonalViewer::setOrientationToXY);
		}

		OrthogonalViewer::~OrthogonalViewer()
		{
			delete this->ui;
		}

		void OrthogonalViewer::setSagittalViewPlaneNormal(double e1, double e2, double e3)
		{
			double normal[] = { e1, e2, e3 };
			if (vtkMath::Norm(normal) < 1e-15) {
				qCritical() << "Attempting to set sagital view plane normal to zero vector.";
				return;
			}
			vtkMath::Normalize(normal);
			if (this->viewPlaneNormalSyncFlag) {
				std::copy(normal, normal + 3, sagittalViewPlaneNormal_);
			}
			else {
				std::copy(normal, normal + 3, this->sagittalViewPlaneNormal);
			}
			/* Check if this is orthogonal with xViewPlaneNormal, if not snap it to
			the closest vect that is orthogonal with xViewPlaneNormal */
			if (qAbs(vtkMath::Dot(this->getSagittalViewPlaneNormal(), this->getAxialViewPlaneNormal())) > 1e-10) {
				double _axialViewPlaneNormal[3];
				if (this->getRighthandness()) {
					vtkMath::Cross(this->getSagittalViewPlaneNormal(), this->getCoronalViewPlaneNormal(), _axialViewPlaneNormal);
				}
				else
				{
					vtkMath::Cross(this->getCoronalViewPlaneNormal(), this->getSagittalViewPlaneNormal(), _axialViewPlaneNormal);
				}
				vtkMath::Normalize(_axialViewPlaneNormal);
				this->setAxialViewPlaneNormal(_axialViewPlaneNormal);
			}
			if (this->getViewPlaneNormalSyncFlag()) {
				for (ViewerList::const_iterator cit = getAllViewers().cbegin();
					cit != getAllViewers().cend(); ++cit) {
					OrthogonalViewer *viewer = qobject_cast<OrthogonalViewer*>(*cit);
					if (viewer)
					{
						viewer->UpdateCameraViewPlaneNormal();
					}
				}
			}
			else {
				this->UpdateCameraViewPlaneNormal();
			}
		}

		void OrthogonalViewer::setSagittalViewPlaneNormal(double *normal)
		{
			OrthogonalViewer::setSagittalViewPlaneNormal(normal[0], normal[1], normal[2]);
		}

		const double *OrthogonalViewer::getAxialViewPlaneNormal() const
		{
			if (this->viewPlaneNormalSyncFlag) {
				return axialViewPlaneNormal_;
			}
			else {
				return this->axialViewPlaneNormal;
			}
		}

		void OrthogonalViewer::getAxialViewPlaneNormal(double normal[3]) const
		{
			std::copy(this->getAxialViewPlaneNormal(), this->getAxialViewPlaneNormal() + 3, normal);
		}

		const double *OrthogonalViewer::getSagittalViewPlaneNormal() const
		{
			if (this->viewPlaneNormalSyncFlag) {
				return sagittalViewPlaneNormal_;
			}
			else {
				return this->sagittalViewPlaneNormal;
			}
		}

		void OrthogonalViewer::getSagittalViewPlaneNormal(double normal[3]) const
		{
			std::copy(this->getSagittalViewPlaneNormal(), this->getSagittalViewPlaneNormal() + 3, normal);
		}

		const double* OrthogonalViewer::getCoronalViewPlaneNormal() const
		{
			//double dotProduct = qAbs(vtkMath::Dot(this->getSagittalViewPlaneNormal(), this->getAxialViewPlaneNormal()));
			//if (dotProduct > 1e-10) {
			//	qWarning() << "SAGITTAL view plane normal and axial view plane normal are almowt ";
			//	qWarning() << "The product of them is " << dotProduct;
			//}
			if (this->viewPlaneNormalSyncFlag) {
				if (this->getRighthandness()) {
					vtkMath::Cross(this->getAxialViewPlaneNormal(), this->getSagittalViewPlaneNormal(), coronalViewPlaneNormal_);
				}
				else {
					vtkMath::Cross(this->getSagittalViewPlaneNormal(), this->getAxialViewPlaneNormal(), coronalViewPlaneNormal_);
				}
				vtkMath::Normalize(coronalViewPlaneNormal_);
				//qDebug() << "sagitial view plane normal";
				//qDebug() << sagittalViewPlaneNormal_[0] << sagittalViewPlaneNormal_[1] << sagittalViewPlaneNormal_[2];
				//qDebug() << "CORONAL View Plane Normal ";
				//qDebug() << coronalViewPlaneNormal_[0] << coronalViewPlaneNormal_[1] << coronalViewPlaneNormal_[2];
				//qDebug() << "axial view plane normal. ";
				//qDebug() << axialViewPlaneNormal_[0] << axialViewPlaneNormal_[1] << axialViewPlaneNormal_[2];
				return coronalViewPlaneNormal_;
			}
			else {
				if (this->getRighthandness()) {
					vtkMath::Cross(this->getAxialViewPlaneNormal(), this->getSagittalViewPlaneNormal(), this->coronalViewPlaneNormal);
				}
				else {
					vtkMath::Cross(this->getSagittalViewPlaneNormal(), this->getAxialViewPlaneNormal(), this->coronalViewPlaneNormal);
				}
				vtkMath::Normalize(this->coronalViewPlaneNormal);
				//qDebug() << "sagitial view plane normal";
				//qDebug() << sagittalViewPlaneNormal[0] << sagittalViewPlaneNormal[1] << sagittalViewPlaneNormal[2];
				//qDebug() << "CORONAL View Plane Normal ";
				//qDebug() << coronalViewPlaneNormal[0] << coronalViewPlaneNormal[1] << coronalViewPlaneNormal[2];
				//qDebug() << "axial view plane normal. ";
				//qDebug() << axialViewPlaneNormal[0] << axialViewPlaneNormal[1] << axialViewPlaneNormal[2];
				return this->coronalViewPlaneNormal;
			}
		}

		void OrthogonalViewer::getCurrentPlaneNormal(double normal[3]) const
		{
			std::copy(this->getCurrentPlaneNormal(), this->getCurrentPlaneNormal() + 3, normal);
		}

		const double* OrthogonalViewer::UpdateCameraViewPlaneNormal()
		{
			switch (this->orientation)
			{
			case SAGITTAL:
				std::copy(this->getSagittalViewPlaneNormal(), this->getSagittalViewPlaneNormal() + 3, this->currentPlaneNormal);
				break;
			case CORONAL:
				std::copy(this->getCoronalViewPlaneNormal(), this->getCoronalViewPlaneNormal() + 3, this->currentPlaneNormal);
				break;
			case AXIAL:
				std::copy(this->getAxialViewPlaneNormal(), this->getAxialViewPlaneNormal() + 3, this->currentPlaneNormal);
				break;
			case ORIENTATION_YZ:
				std::copy(ORIENTATION_YZ_NORMAL, ORIENTATION_YZ_NORMAL + 3, this->currentPlaneNormal);
				break;
			case ORIENTATION_XZ:
				std::copy(ORIENTATION_XZ_NORMAL, ORIENTATION_XZ_NORMAL + 3, this->currentPlaneNormal);
				break;
			case ORIENTATION_XY:
				std::copy(ORIENTATION_XY_NORMAL, ORIENTATION_XY_NORMAL + 3, this->currentPlaneNormal);
				break;
			default:
				break;
			}
			/* set new position to camera */
			double dist = this->getActiveCamera()->GetDistance();
			double *fp = this->getActiveCamera()->GetFocalPoint();
			double newCamPos[3];
			double targetOri[3];
			std::copy(this->currentPlaneNormal, this->currentPlaneNormal + 3, targetOri);
			vtkMath::MultiplyScalar(targetOri, dist);
			vtkMath::Add(fp, targetOri, newCamPos);
			this->getActiveCamera()->SetPosition(newCamPos);
			this->UpdateViewUp();
			return this->currentPlaneNormal;
		}

		double* OrthogonalViewer::UpdateViewUp()
		{
			switch (this->orientation) {
			case SAGITTAL:
				this->getActiveCamera()->SetViewUp(this->getAxialViewPlaneNormal());
				break;
			case CORONAL:
				this->getActiveCamera()->SetViewUp(this->getSagittalViewPlaneNormal());
				break;
			case AXIAL:
				this->getActiveCamera()->SetViewUp(this->getCoronalViewPlaneNormal());
				break;
			case ORIENTATION_YZ:
				this->getActiveCamera()->SetViewUp(ORIENTATION_XY_NORMAL);
				break;
			case ORIENTATION_XZ:
				this->getActiveCamera()->SetViewUp(ORIENTATION_YZ_NORMAL);
				break;
			case ORIENTATION_XY:
				this->getActiveCamera()->SetViewUp(ORIENTATION_XZ_NORMAL);
				break;
			default:
				std::string errmsg = "ORIENTATION is not between 0 to 2!";
				qCritical() << QString::fromStdString(errmsg);
				break;
			}
			return this->getActiveCamera()->GetViewUp();
		}

		void OrthogonalViewer::setAxialViewPlaneNormal(double *normal)
		{
			OrthogonalViewer::setAxialViewPlaneNormal(normal[0], normal[1], normal[2]);
		}

		void OrthogonalViewer::setAxialViewPlaneNormal(double e1, double e2, double e3)
		{
			double normal[] = { e1, e2, e3 };
			if (vtkMath::Norm(normal) < 1e-15) {
				qCritical() << "Attempting to set axial view plane normal to zero vector.";
				return;
			}
			vtkMath::Normalize(normal);
			if (this->viewPlaneNormalSyncFlag) {
				std::copy(normal, normal + 3, axialViewPlaneNormal_);
			}
			else {
				std::copy(normal, normal + 3, this->axialViewPlaneNormal);
			}
			if (qAbs(vtkMath::Dot(this->getSagittalViewPlaneNormal(), this->getAxialViewPlaneNormal())) > 1e-10) {
				double _sagitalViewPlaneNormal[3];
				if (this->getRighthandness()) {
					vtkMath::Cross(this->getCoronalViewPlaneNormal(), this->getAxialViewPlaneNormal(), _sagitalViewPlaneNormal);
				}
				else {
					vtkMath::Cross(this->getAxialViewPlaneNormal(), this->getCoronalViewPlaneNormal(), _sagitalViewPlaneNormal);
				}
				vtkMath::Normalize(_sagitalViewPlaneNormal);
				this->setSagittalViewPlaneNormal(_sagitalViewPlaneNormal);
			}
			if (this->getViewPlaneNormalSyncFlag()) {
				for (Viewer *viewer : getAllViewers()) {
					OrthogonalViewer *orthogonalViewer = qobject_cast<OrthogonalViewer*>(viewer);
					if (orthogonalViewer) {
						orthogonalViewer->UpdateCameraViewPlaneNormal();
					}
				}
			}
			else {
				this->UpdateCameraViewPlaneNormal();
			}
		}

		vtkRenderWindow *OrthogonalViewer::getRenderWindow()
		{
			return this->ui->qvtkWidget->GetRenderWindow();
		}

		vtkRenderWindowInteractor *OrthogonalViewer::getInteractor()
		{
			return this->ui->qvtkWidget->GetInteractor();
		}

		void OrthogonalViewer::setRighthandness(bool b)
		{
			if (this->viewPlaneNormalSyncFlag) {
				righthandness_ = b;
			}
			else {
				this->righthandness = b;
			}
		}

		bool OrthogonalViewer::getRighthandness() const
		{
			if (this->viewPlaneNormalSyncFlag) {
				return righthandness_;
			}
			else {
				return this->righthandness;
			}
		}
		void OrthogonalViewer::setViewPlaneNormalSyncFlag(bool flag)
		{
			this->viewPlaneNormalSyncFlag = flag;
			this->UpdateCameraViewPlaneNormal();
		}
	}
}

