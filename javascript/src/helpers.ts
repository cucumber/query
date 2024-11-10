interface WithId {
    id: string | number
}

export function comparatorById(a: WithId, b: WithId) {
    return comparatorBy(a, b, "id")
}

export function comparatorBy(a: any, b: any, key: string) {
    if ( a[key] < b[key] ){
        return -1;
    }
    if ( a[key] > b[key] ){
        return 1;
    }
    return 0;
}