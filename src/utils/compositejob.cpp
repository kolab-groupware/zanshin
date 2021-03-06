/* This file is part of Zanshin

   Copyright 2014 Mario Bensi <mbensi@ipsquad.net>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
   USA.
*/

#include "compositejob.h"

using namespace Utils;

CompositeJob::CompositeJob(QObject *parent)
    : KCompositeJob(parent)
{

}

void CompositeJob::start()
{
    if (hasSubjobs()) {
        subjobs().first()->start();
    } else {
        emitResult();
    }
}

/**
  * gcc 4.7 does not like overloaded functions with different types of lambdas.
  * because the second variant is not used at all till now disable it.
bool CompositeJob::install(KJob *job, const JobHandler::ResultHandlerWithJob &handler)
{
    if (!addSubjob(job))
        return false;

    JobHandler::installWithJob(job, handler);
    return true;
}
*/

bool CompositeJob::install(KJob *job, const JobHandler::ResultHandler &handler)
{
    JobHandler::install(job, handler);

    if (!addSubjob(job))
        return false;

    return true;
}

void CompositeJob::slotResult(KJob *job)
{
    KCompositeJob::slotResult(job);

    if (!hasSubjobs())
        emitResult();
}
