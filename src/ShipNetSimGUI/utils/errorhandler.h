#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H

#include <QString>

namespace ErrorHandler {

/**
     * @brief Displays a notification message.
     *
     * @param msg The notification message to be displayed.
     */
void showNotification(QString msg);

/**
     * @brief Displays a warning message.
     *
     * @param msg The warning message to be displayed.
     */
void showWarning(QString msg);

/**
     * @brief Displays an error message.
     *
     * @param msg The error message to be displayed.
     */
void showError(QString msg);

}


#endif // ERRORHANDLER_H
