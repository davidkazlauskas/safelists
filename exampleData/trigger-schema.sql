CREATE TRIGGER after_insert_files AFTER INSERT ON files
BEGIN
    UPDATE metadata
    SET revision_number=revision_number+1,modification_date=strftime('%s','now');
END;

CREATE TRIGGER after_delete_files AFTER DELETE ON files
BEGIN
    UPDATE metadata
    SET revision_number=revision_number+1,modification_date=strftime('%s','now');
END;


CREATE TRIGGER after_update_files AFTER UPDATE ON files
BEGIN
    UPDATE metadata
    SET revision_number=revision_number+1,modification_date=strftime('%s','now');
END;


CREATE TRIGGER after_insert_directories AFTER INSERT ON directories
BEGIN
    UPDATE metadata
    SET revision_number=revision_number+1,modification_date=strftime('%s','now');
END;

CREATE TRIGGER after_delete_directories AFTER DELETE ON directories
BEGIN
    UPDATE metadata
    SET revision_number=revision_number+1,modification_date=strftime('%s','now');
END;


CREATE TRIGGER after_update_directories AFTER UPDATE ON directories
BEGIN
    UPDATE metadata
    SET revision_number=revision_number+1,modification_date=strftime('%s','now');
END;



CREATE TRIGGER after_insert_mirrors AFTER INSERT ON mirrors
BEGIN
    UPDATE metadata
    SET revision_number=revision_number+1,modification_date=strftime('%s','now');
END;

CREATE TRIGGER after_delete_mirrors AFTER DELETE ON mirrors
BEGIN
    UPDATE metadata
    SET revision_number=revision_number+1,modification_date=strftime('%s','now');
END;


CREATE TRIGGER after_update_mirrors AFTER UPDATE OF file_id, url ON mirrors
BEGIN
    UPDATE metadata
    SET revision_number=revision_number+1,modification_date=strftime('%s','now');
END;

