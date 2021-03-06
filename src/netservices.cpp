#include <string>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QEventLoop>
#include <QUuid>

#include "netservices.h"
#include "account.h"
#include "transfer.h"

QString baseApiUrl = "https://demo.integralces.net/ces/api";

netServices::netServices(QObject *parent)
    : QObject(parent),
    netManager(new QNetworkAccessManager(this)),
    oauth2(netManager),
    accounts(),
    index_current_acc(0),
    comma_list("")
{}

netServices::~netServices() { delete netManager; }

void netServices::get_accounts() {
    QNetworkRequest networkRequest(
        QUrl(baseApiUrl + "/social/users/me?include=members"));
    oauth2.prepare_request(networkRequest);

    netManager->get(networkRequest);
    connect(netManager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(get_accounts_reply(QNetworkReply*)), Qt::SingleShotConnection);
}

void netServices::get_accounts_reply(QNetworkReply* getReply) {
    if(getReply->error()) {
        qDebug() << "Error: " << getReply->errorString();
        getReply->deleteLater();
        emit network_error();
        }
    else {
        QString strReply = getReply->readAll();
        getReply->deleteLater();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());
        QString user_id = jsonResponse.object()["data"].toObject()["id"].toString();
        int sizeArray = jsonResponse.object()["included"].toArray().size();

        QJsonObject acc;
        for (int i=0; i < sizeArray; i++) {
            acc = jsonResponse.object()["included"].toArray()[i].toObject();
            accounts.push_back(account(user_id, acc["id"].toString()));
            accounts[i].member_name = acc["attributes"].toObject()["name"].
                    toString();
            accounts[i].member_image = acc["attributes"].toObject()["image"].
                    toString();
            accounts[i].account_id = acc["relationships"].toObject()["account"].
                    toObject()["data"].toObject()["id"].toString();
            accounts[i].account_code = acc["attributes"].toObject()["code"].
                    toString();
            accounts[i].account_link = acc["relationships"].toObject()["account"].
                    toObject()["links"].toObject()["related"].toString();
            accounts[i].group_id = acc["relationships"].toObject()["group"].
                    toObject()["data"].toObject()["id"].toString();
            accounts[i].group_code = accounts[i].account_code.left(4);
        };
        index_current_acc = 0;
        emit has_accounts();
    }
}

void netServices::get_account_balance() {
    QNetworkRequest networkRequest(
        QUrl(accounts[index_current_acc].account_link + "?include=currency"));
    oauth2.prepare_request(networkRequest);

    netManager->get(networkRequest);
    connect(netManager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(get_account_balance_reply(QNetworkReply*)), Qt::SingleShotConnection);
}

void netServices::get_account_balance_reply(QNetworkReply* getReply) {
    if(getReply->error()) {
        qDebug() << "Error: " << getReply->errorString();
        getReply->deleteLater();
        emit network_error();
    }
    else {
        QString strReply = getReply->readAll();
        getReply->deleteLater();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());
        accounts[index_current_acc].balance = jsonResponse.object()["data"].toObject()["attributes"].
                toObject()["balance"].toDouble();
        accounts[index_current_acc].currency.id =  jsonResponse.object()["included"].toArray()[0].
                toObject()["id"].toString();
        QJsonObject attr = jsonResponse.object()["included"].toArray()[0].
                toObject()["attributes"].toObject();
        accounts[index_current_acc].currency.name = attr["name"].toString();
        accounts[index_current_acc].currency.plural = attr["namePlural"].toString();
        accounts[index_current_acc].currency.symbol = attr["symbol"].toString();
        accounts[index_current_acc].currency.decimals = attr["decimals"].toString().toInt();
        emit has_balance();
    }
}

void netServices::get_account_transfers() {
    QString url = baseApiUrl + "/accounting/" +
            accounts[index_current_acc].group_code +
            "/transfers?filter[account]=" +
            accounts[index_current_acc].account_id +
            "&sort=-created";
    QNetworkRequest networkRequest(url);
    oauth2.prepare_request(networkRequest);

    netManager->get(networkRequest);
    connect(netManager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(get_account_transfers_reply(QNetworkReply*)), Qt::SingleShotConnection);

}

void netServices::get_account_transfers_reply(QNetworkReply* getReply) {
    if(getReply->error()) {
        qDebug() << "Error: " << getReply->errorString();
        getReply->deleteLater();
        emit network_error();
    }
    else {
        QString strReply = getReply->readAll();
        getReply->deleteLater();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());
        int sizeArray = jsonResponse.object()["data"].toArray().size();

        if (!accounts[index_current_acc].transfers.empty()) {
            accounts[index_current_acc].transfers.clear();
        }

        QJsonObject trans;
        std::vector<QString> unknown_accounts;
        for (int i=0; i < sizeArray; i++) {
            trans = jsonResponse.object()["data"].toArray()[i].toObject();
            accounts[index_current_acc].transfers.push_back(transfer(trans["id"].toString()));
            transfer* p = &accounts[index_current_acc].transfers.back();
            p->amount = trans["attributes"].toObject()["amount"].toInt();
            p->meta = trans["attributes"].toObject()["meta"].toString();
            p->state = trans["attributes"].toObject()["state"].toString();
            p->created = trans["attributes"].toObject()["created"].toString();
            p->updated = trans["attributes"].toObject()["updated"].toString();
            p->payer_account_id = trans["relationships"].toObject()["payer"].
                    toObject()["data"].toObject()["id"].toString();
            if (p->payer_account_id == accounts[index_current_acc].account_id) {
                p->payer_account_code = accounts[index_current_acc].account_code;
            }
            else {unknown_accounts.push_back(p->payer_account_id);}
            p->payee_account_id = trans["relationships"].toObject()["payee"].
                    toObject()["data"].toObject()["id"].toString();
            if (p->payee_account_id == accounts[index_current_acc].account_id) {
                p->payee_account_code = accounts[index_current_acc].account_code;
            }
            else {unknown_accounts.push_back(p->payee_account_id);}
            p->currency.id = trans["relationships"].toObject()["currency"].
                    toObject()["data"].toObject()["id"].toString();
            if (p->currency.id == accounts[index_current_acc].currency.id) {
                p->currency = accounts[index_current_acc].currency;
            }
        }
        for(int j= 0; j < (int)unknown_accounts.size(); j++) {
            comma_list += unknown_accounts[j];
            if (unknown_accounts[j] != unknown_accounts.back()) comma_list += ",";
        }
        emit has_transfers();
    }

}

void netServices::get_unknown_accounts() {
    QString url = baseApiUrl + "/social/" + accounts[index_current_acc].group_code +
            "/members?filter[account]=" + comma_list;
    QNetworkRequest networkRequest(url);
    oauth2.prepare_request(networkRequest);

    netManager->get(networkRequest);
    connect(netManager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(get_unknown_accounts_reply(QNetworkReply*)), Qt::SingleShotConnection);
}

void netServices::get_unknown_accounts_reply(QNetworkReply* getReply) {
    if(getReply->error()) {
        qDebug() << "Error: " << getReply->errorString();
        getReply->deleteLater();
        emit network_error();
    }
    else {
        QString strReply = getReply->readAll();
        getReply->deleteLater();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());
        int sizeArray = jsonResponse.object()["data"].toArray().size();
        QJsonObject da;
        for (int i=0; i < sizeArray; i++) {
            da = jsonResponse.object()["data"].toArray()[i].toObject();
            QString id = da["relationships"].toObject()["account"].toObject()
                    ["data"].toObject()["id"].toString();
            for (int j=0; j < (int)accounts[index_current_acc].transfers.size(); j++) {
                transfer* p = &accounts[index_current_acc].transfers[j];
                if (id == p->payer_account_id) {
                    p->payer_account_code = da["attributes"].toObject()["code"].
                            toString();
                } else {
                    if (id == p->payee_account_id) {
                        p->payee_account_code = da["attributes"].toObject()["code"].
                            toString();
                    }
                }
            }
        }
        emit has_all_data();
    }
}

void netServices::get_check_account(const QString& group_code, const QString& account_code) {
    QString url = baseApiUrl + "/accounting/" + group_code + "/accounts/" + account_code;
    QNetworkRequest networkRequest(url);
    oauth2.prepare_request(networkRequest);

    netManager->get(networkRequest);
    connect(netManager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(get_check_account_reply(QNetworkReply*)), Qt::SingleShotConnection);
}

void netServices::get_check_account_reply(QNetworkReply* getReply) {
    if(getReply->error()) {
        qDebug() << "Error: " << getReply->errorString();
        getReply->deleteLater();
        emit network_error();
    }
    else {
        QString strReply = getReply->readAll();
        getReply->deleteLater();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());

        newTrans = new transfer(QUuid::createUuid().toString(QUuid::WithoutBraces));
        newTrans->amount = 0;
        newTrans->meta = "";
        newTrans->state = "committed";
        newTrans->created = "";
        newTrans->updated = "";
        newTrans->payer_account_id = jsonResponse.object()["data"].toObject()["id"].toString();
        newTrans->payer_account_code = jsonResponse.object()["data"].toObject()["attributes"].toObject()["code"].toString();;
        newTrans->payee_account_id = accounts[index_current_acc].account_id;
        newTrans->payee_account_code = accounts[index_current_acc].account_code;
        newTrans->currency = accounts[index_current_acc].currency;

        emit confirm_transfer();
    }
}

void netServices::post_new_transfer() {

    // data to post
    QString payload = QString("{"
        "\"data\": {"
            "\"id\": \"%1\","
            "\"type\": \"transfers\","
            "\"attributes\": {"
                "\"amount\": %2,"
                "\"meta\": \"%3\","
                "\"state\": \"committed\""
            "},"
            "\"relationships\": {"
                "\"payer\": {"
                    "\"data\": {"
                        "\"type\": \"accounts\","
                        "\"id\": \"%4\""
                    "}"
                "},"
                "\"payee\": {"
                    "\"data\": {"
                        "\"type\": \"accounts\","
                        "\"id\": \"%5\""
                    "}"
                "}"
            "}"
        "}"
    "}").arg(
        newTrans->id,
        QString::number(newTrans->amount),
        newTrans->meta,
        newTrans->payer_account_id,
        newTrans->payee_account_id
    );
    QJsonDocument json_to_post = QJsonDocument::fromJson(payload.toUtf8());
    QByteArray postData = json_to_post.toJson();

    QUrl url = QUrl(baseApiUrl + "/accounting/" + accounts[index_current_acc].group_code + "/transfers");
    QNetworkRequest networkRequest(url);

    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader,
                             "application/json");
    networkRequest.setRawHeader("Authorization",
                                QString("Bearer " + oauth2.kSettingsP->access_token).toUtf8());

    netManager->post(networkRequest, postData);
    connect(netManager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(post_new_transfer_reply(QNetworkReply*)), Qt::SingleShotConnection);
}

void netServices::post_new_transfer_reply(QNetworkReply* postReply) {
    if(postReply->error()) {
        qDebug() << "Error: " << postReply->errorString();
        postReply->deleteLater();
        emit network_error();
    }
    else {
        QString strReply = postReply->readAll();
        postReply->deleteLater();
        emit transfer_done();
    }
}
