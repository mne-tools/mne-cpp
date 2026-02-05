
//=============================================================================================================

void BrainSurface::boundingBox(QVector3D &min, QVector3D &max) const
{
    if (m_vertexData.isEmpty()) {
        min = QVector3D(0,0,0);
        max = QVector3D(0,0,0);
        return;
    }

    min = m_vertexData[0].pos;
    max = m_vertexData[0].pos;

    for (const auto &v : m_vertexData) {
        if (v.pos.x() < min.x()) min.setX(v.pos.x());
        if (v.pos.y() < min.y()) min.setY(v.pos.y());
        if (v.pos.z() < min.z()) min.setZ(v.pos.z());
        
        if (v.pos.x() > max.x()) max.setX(v.pos.x());
        if (v.pos.y() > max.y()) max.setY(v.pos.y());
        if (v.pos.z() > max.z()) max.setZ(v.pos.z());
    }
}
