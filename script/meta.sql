CREATE TABLE filemeta(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    path TEXT NOT NULL,
    ctime INTEGER NOT NULL
);
CREATE INDEX idx_filemeta_path ON filemeta(path);
CREATE TABLE space(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    writtenbytes INTEGER DEFAULT 0,
    diskbytes INTEGER DEFAULT 0,
    filenum INTEGER DEFAULT 0
);
INSERT INTO space(writtenbytes, diskbytes, filenum) values(0, 0, 0);
