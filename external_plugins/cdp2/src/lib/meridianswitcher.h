#ifndef MERIDIANSWITCHER_H_
#define MERIDIANSWITCHER_H_
#include <QObject>
class QString;

/** \ingroup library
 * \brief This class will shift the meridian (where greenwich meridian is found in the leftmost column)
 * of aarcinfo grid file.....later maybe any gdal file that supports creation.
 */
class CDP_LIB_EXPORT  MeridianSwitcher : public QObject
{

Q_OBJECT;

public:

  /** Default constructor */
  MeridianSwitcher();
  /** Destructor  */
   ~MeridianSwitcher();
   /** Dis waar die kak aangejaag word.... */
  bool doSwitch(QString theInputFileString, QString theOutputFileString);

signals: 
  void error(QString theError);
  void message(QString theMessage);

private:
  //
  //   Private attributes
  //

};

#endif  //MERIDIANSWITCHER

