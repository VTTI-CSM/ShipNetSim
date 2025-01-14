#include "customplot.h"
#include <algorithm>

// Constructor
CustomPlot::CustomPlot(QWidget *parent) :
    QCustomPlot(parent), m_isPanning(false), m_isScrollButtonClicked(false)
{
    // Enable interaction
    setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    // Add a default graph
    addGraph(xAxis, yAxis);

    // Set default axis labels
    xAxis->setLabel("X-Axis");
    yAxis->setLabel("Y-Axis");

    // Set default legend visibility
    legend->setVisible(true);

    // Enable interaction
    QObject::connect(this, SIGNAL(zoomReset()), this, SLOT(resetZoom()));
}

// Mouse press event
void CustomPlot::mousePressEvent(QMouseEvent *event)
{
    // Emit signals for left and right button press
    if (event->button() == Qt::LeftButton) {
        emit this->pointLeftSelected(this->getClosestPoint(event));
    }
    else if (event->button() == Qt::RightButton) {
        emit this->pointRightSelected(this->getClosestPoint(event));
    }

    QCustomPlot::mousePressEvent(event);
}

// Mouse double click event
void CustomPlot::mouseDoubleClickEvent(QMouseEvent *event)
{
    // Reset zoom on double click
    if (event->button()== Qt::MiddleButton) {
        emit zoomReset();
    }

    QCustomPlot::mouseDoubleClickEvent(event);
}

// Center the drawing
void CustomPlot::centerDrawing()
{
    // Calculate the center coordinates of the graph
    double centerX = (xAxis->range().lower + xAxis->range().upper) / 2.0;
    double centerY = (yAxis->range().lower + yAxis->range().upper) / 2.0;

    // Set new axis ranges to center the drawing
    xAxis->setRange(
        centerX - (xAxis->range().size() / 2.0),
        centerX + (xAxis->range().size() / 2.0));
    yAxis->setRange(
        centerY - (yAxis->range().size() / 2.0),
        centerY + (yAxis->range().size() / 2.0));

    replot();
}

// Reset zoom
void CustomPlot::resetZoom()
{
    if (graphCount() < 1) {
        return;
    }

    QVector<double> minXV, maxXV, minYV, maxYV;

    for (int i = 0; i < graphCount(); i++) {
        auto data = this->getAllPointsPositions(*graph(i));
        auto xdata = data.first;
        auto ydata = data.second;

        if (xdata.size() < 1 || ydata.size() < 1) {
            continue;  // Skip empty data sets
        }

        // Get the maximum and minimum values of the plotted points
        minXV.push_back(*std::min_element(xdata.begin(), xdata.end()));
        maxXV.push_back(*std::max_element(xdata.begin(), xdata.end()));
        minYV.push_back(*std::min_element(ydata.begin(), ydata.end()));
        maxYV.push_back(*std::max_element(ydata.begin(), ydata.end()));
    }

    if (minXV.empty() || maxXV.empty() || minYV.empty() || maxYV.empty()) {
        return;  // Skip if all vectors are empty
    }

    double minX = *std::min_element(minXV.begin(), minXV.end());
    double maxX = *std::max_element(maxXV.begin(), maxXV.end());
    double minY = *std::min_element(minYV.begin(), minYV.end());
    double maxY = *std::max_element(maxYV.begin(), maxYV.end());

    // Calculate the percentage of the difference
    double xRange = (maxX - minX) * 0.1;  // Adjust the percentage as desired
    double yRange = (maxY - minY) * 0.1;  // Adjust the percentage as desired

    // Set the range based on the maximum and minimum values
    xAxis->setRange(minX - xRange, maxX + xRange);
    yAxis->setRange(minY - yRange, maxY + yRange);

    replot();
}


// Get all points positions of a graph
QPair<QVector<double>,
      QVector<double>> CustomPlot::getAllPointsPositions(QCPGraph &graph)
{
    QVector<double> xdata;
    QVector<double> ydata;

    // Get the data of the graph
    QSharedPointer< QCPGraphDataContainer >  data = graph.data();

    // Iterate over the data points of the graph
    for (auto &graphData : *data)
    {
        double x = graphData.key;
        double y = graphData.value;
        xdata.push_back(x);
        ydata.push_back(y);
    }
    return std::make_pair(xdata, ydata);
}

// Get the closest point to a mouse event
QPointF CustomPlot::getClosestPoint(QMouseEvent *event)
{
    // Convert the click position to coordinates in the plot
    double clickX = xAxis->pixelToCoord(event->pos().x());
    double clickY = yAxis->pixelToCoord(event->pos().y());

    // Iterate through each graph to find the closest point
    double closestDistance = std::numeric_limits<double>::max();
    QPointF closestPoint;

    bool pointFound = false; // Flag to check if a valid point is found

    for (int i = 0; i < graphCount(); i++) {
        auto currentGraph = graph(i);

        if (currentGraph) {
            QSharedPointer<QCPGraphDataContainer> dataContainer =
                currentGraph->data();

            double maxRangeX = xAxis->range().size();
            double maxRangeY = yAxis->range().size();
            double maxDistance = std::hypot(maxRangeX, maxRangeY) * 0.1;

            // Iterate through the data points of the graph
            for (int j = 0; j < dataContainer->size(); j++) {
                double x = dataContainer.data()->at(j)->key;
                double y = dataContainer.data()->at(j)->value;

                // Calculate the distance between the
                // clicked position and the data point
                double distance = std::hypot(x - clickX, y - clickY);

                // if distance is more than max distance, skip
                if (distance > maxDistance) {
                    continue;
                }

                // Update the closest point if this distance is smaller
                if (distance < closestDistance) {
                    closestDistance = distance;
                    closestPoint.setX(x);
                    closestPoint.setY(y);
                    // Set flag to true indicating a point is found
                    pointFound = true;
                }
            }
        }
    }

    if (!pointFound) {
        // Return a special point to indicate that no point was found
        closestPoint.setX(std::numeric_limits<double>::quiet_NaN());
        closestPoint.setY(std::numeric_limits<double>::quiet_NaN());
    }

    return closestPoint;
}

void CustomPlot::drawLineGraph(const QVector<double>& xData,
                               const QVector<double>& yData,
                               const QString& xLabel,
                               const QString& yLabel,
                               const QString& graphName,
                               int plotIndex)
{
    if (xData.isEmpty() || yData.isEmpty()) {
        throw std::invalid_argument("Error: xData and yData must "
                                    "not be empty.");

    }
    if (xData.size() != yData.size()) {
        throw std::invalid_argument("Size mismatch: xData and yData must "
                                    "have the same size.");
    }
    // Ensure there are enough graphs in the plot
    while (graphCount() <= plotIndex) {
        addGraph(xAxis, yAxis);
    }

    // Access the graph at the specified index
    QCPGraph* graph = this->graph(plotIndex);
    if (!graph) {
        return; // Safety check
    }

    // Set the data for the graph
    graph->setData(xData, yData);

    // Set axis labels
    xAxis->setLabel(xLabel);
    yAxis->setLabel(yLabel);

    // Define different colors for each plotIndex
    static const QVector<QColor> plotColors = {
        Qt::blue, Qt::red, Qt::green,
        Qt::cyan, Qt::magenta, Qt::darkBlue,
        Qt::darkRed, Qt::darkGreen, Qt::darkCyan,
        Qt::darkMagenta, Qt::darkYellow, Qt::yellow
    };

    // Set the pen and line style for the graph
    QPen pen(plotColors[plotIndex % plotColors.size()]);
    pen.setWidth(2);
    graph->setPen(pen);
    graph->setLineStyle(QCPGraph::lsLine);

    // Make the legend visible and set the graph's name
    legend->setVisible(true);
    graph->setName(graphName);

    // Adjust the axis ranges and replot
    resetZoom(); // Reset zoom before showing new data
    replot();    // Update the plot
}

