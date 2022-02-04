#ifndef QUOTEDTO_H
#define QUOTEDTO_H

#include <QObject>

class QuoteDTO {
    Q_GADGET
    Q_PROPERTY( QString text READ text WRITE setText )
    Q_PROPERTY( QString author READ author WRITE setAuthor )

public:
    QuoteDTO() = default;

    const QString& text() const { return m_text; }
    void setText( const QString& text ) { m_text = text; }

    const QString& author() const { return m_author; }
    void setAuthor( const QString& author ) { m_author = author; }

private:
    QString m_text;
    QString m_author;

};

#endif // QUOTEDTO_H
