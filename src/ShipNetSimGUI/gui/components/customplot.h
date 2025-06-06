#ifndef CUSTOMPLOT_H
#define CUSTOMPLOT_H

#include "../third_party/qcustomplot/qcustomplot.h"


/**
 * @brief The CustomPlot class is a subclass of QCustomPlot,
 * providing additional functionality and customization.
 *
 * This class extends the QCustomPlot widget to handle mouse
 * events and zoom reset functionality.
 * It also includes methods for panning, retrieving points
 * positions, and emitting signals for selected points.
 *
 * @author Ahmed Aredah
 * @date 6/7/2023
 */
class CustomPlot : public QCustomPlot
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a CustomPlot object.
     * @param parent The parent widget.
     *
     * @author Ahmed Aredah
    * @date 6/7/2023
     */
    explicit CustomPlot(QWidget *parent = nullptr);

    void drawLineGraph(const QVector<double>& xData,
                       const QVector<double>& yData,
                       const QString& xLabel,
                       const QString& yLabel,
                       const QString& graphName,
                       int plotIndex);

protected:
    /**
     * @brief Reimplemented mousePressEvent from QCustomPlot.
     * @param event The mouse press event.
     *
     * @author Ahmed Aredah
     * @date 6/7/2023
     */
    virtual void mousePressEvent(QMouseEvent *event) override;

    /**
     * @brief Reimplemented mouseDoubleClickEvent from QCustomPlot.
     * @param event The mouse double-click event.
     *
     * @author Ahmed Aredah
     * @date 6/7/2023
     */
    virtual void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    bool m_isPanning;                 // Flag to indicate if panning
        // is in progress
    double m_x0, m_y0;                // Initial coordinates for panning
    double m_xPress, m_yPress;        // Press coordinates for panning
    double m_curXRange, m_curYRange;  // Current axis ranges
    bool m_isScrollButtonClicked;     // Flag to indicate if the
        // scroll button is clicked

    qint64  zoomResetTimeStamp;       // Time stamp for last zoom reset
    int zoomResetClickCounter;        // Counter for consecutive
        // zoom reset clicks

    // Sensitivity factor for panning
    std::pair<double, double> panningSensitivity = std::make_pair(1.0, 1.0);

private:
    /**
     * @brief Centers the drawing in the plot.
     *
     * @author Ahmed Aredah
     * @date 6/7/2023
     */
    void centerDrawing();

    /**
     * @brief Retrieves the positions of all points in a graph.
     * @param graph The QCPGraph object representing the graph.
     * @return A pair of vectors representing the x and y
     * positions of the points, respectively.
     *
     * @author Ahmed Aredah
     * @date 6/7/2023
     */
    QPair<QVector<double>, QVector<double>> getAllPointsPositions(
        QCPGraph &graph);

    /**
     * @brief Retrieves the closest data point to the mouse event position.
     * @param event The mouse event.
     * @return The position of the closest point.
     *
     * @author Ahmed Aredah
     * @date 6/7/2023
     */
    QPointF getClosestPoint(QMouseEvent *event);

signals:
    /**
     * @brief Signal emitted when the zoom is reset.
     *
     * @author Ahmed Aredah
     * @date 6/7/2023
     */
    void zoomReset();

    /**
     * @brief Signal emitted when a data point on the left side is selected.
     * @param point The position of the selected point.
     *
     * @author Ahmed Aredah
     * @date 6/7/2023
     */
    void pointLeftSelected(QPointF point);

    /**
     * @brief Signal emitted when a data point on the right side is selected.
     * @param point The position of the selected point.
     *
     * @author Ahmed Aredah
     * @date 6/7/2023
     */
    void pointRightSelected(QPointF point);

public slots:
    /**
     * @brief Slot function for handling zoom reset.
     *
     * @author Ahmed Aredah
     * @date 6/7/2023
     */
    void resetZoom();
};

#endif // CUSTOMPLOT_H
